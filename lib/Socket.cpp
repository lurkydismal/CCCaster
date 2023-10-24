#include "SocketManager.hpp"
#include "Socket.hpp"
#include "TcpSocket.hpp"
#include "UdpSocket.hpp"
#include "SmartSocket.hpp"
#include "Exceptions.hpp"
#include "ErrorStrings.hpp"

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <cereal/types/unordered_map.hpp>

#include <unordered_set>
#include <algorithm>

using namespace std;


#define READ_BUFFER_SIZE ( 1024 * 4096 )

#define SET_NON_BLOCKING_MODE(VALUE)                                                                                \
    do {                                                                                                            \
        u_long flag = VALUE;                                                                                        \
        if ( ioctlsocket ( _fd, FIONBIO, &flag ) != 0 ) {                                                           \
            LOG_SOCKET ( this, "ioctlsocket(FIONBIO, %u) failed", VALUE );                                          \
            closesocket ( _fd );                                                                                    \
            _fd = 0;                                                                                                \
            THROW_WIN_EXCEPTION ( WSAGetLastError(), "ioctlsocket(FIONBIO, %u) failed", ERROR_NETWORK_GENERIC );    \
        }                                                                                                           \
    } while ( 0 )


static bool enableForceReusePort = true;


Socket::Socket ( Owner *owner, const IpAddrPort& address, Protocol protocol, bool isRaw )
    : owner ( owner ), address ( address ), protocol ( protocol ), _isRaw ( isRaw )
{
    resetBuffer();
}

Socket::~Socket()
{
    disconnect();
}

void Socket::disconnect()
{
    LOG_SOCKET ( this, "disconnected" );

    if ( _fd )
        closesocket ( _fd );

    owner = 0;
    _state = State::Disconnected;
    _fd = 0;

    freeBuffer();

    _packetLoss = _hashFailRate = 0;
}

void Socket::init()
{
    ASSERT ( _fd == 0 );

    WinException exc;
    shared_ptr<addrinfo> addrInfo;

    // TODO proper binding of IPv6 interfaces
    if ( isClient() && isUDP() )
    {
        // For client UDP sockets, bind to any available local port
        addrInfo = getAddrInfo ( "", 0, true, true );
    }
    else
    {
        // Otherwise bind to the given address and port
        addrInfo = getAddrInfo ( address.addr, address.port, true, isServer() || isUDP() );
    }

    addrinfo *res = addrInfo.get();

    for ( ; res; res = res->ai_next )
    {
        if ( isTCP() )
            _fd = ::socket ( res->ai_family, SOCK_STREAM, IPPROTO_TCP );
        else
            _fd = ::socket ( res->ai_family, SOCK_DGRAM, IPPROTO_UDP );

        if ( _fd == INVALID_SOCKET )
        {
            exc = WinException ( WSAGetLastError(), "socket failed", ERROR_NETWORK_GENERIC );
            _fd = 0;
            LOG_SOCKET ( this, "%s", exc );
            continue;
        }

        if ( enableForceReusePort && ( isServer() || isUDP() ) )
        {
            const char yes = 1;

            // SO_REUSEADDR can replace existing port binds
            // SO_EXCLUSIVEADDRUSE only replaces if not exact match
            if ( setsockopt ( _fd, SOL_SOCKET, SO_REUSEADDR, &yes, 1 ) == SOCKET_ERROR )
            {
                exc = WinException ( WSAGetLastError(), "setsockopt failed", ERROR_NETWORK_GENERIC );
                LOG_SOCKET ( this, "%s", exc );
                // Should be safe to continue even if this fails
            }
        }

        SET_NON_BLOCKING_MODE ( 1 );

        if ( isClient() )
        {
            if ( isTCP() )
            {
                if ( ::connect ( _fd, res->ai_addr, res->ai_addrlen ) == SOCKET_ERROR )
                {
                    int error = WSAGetLastError();

                    // Successful non-blocking connect
                    if ( error == WSAEWOULDBLOCK || error == WSAEINVAL )
                        break;

                    exc = WinException ( error, "connect failed", ERROR_NETWORK_GENERIC );
                    LOG_SOCKET ( this, "%s", exc );
                    closesocket ( _fd );
                    _fd = 0;
                    continue;
                }
            }
            else
            {
                if ( ::bind ( _fd, res->ai_addr, res->ai_addrlen ) == SOCKET_ERROR )
                {
                    exc = WinException ( WSAGetLastError(), "bind failed", ERROR_NETWORK_GENERIC );
                    LOG_SOCKET ( this, "%s", exc );
                    closesocket ( _fd );
                    _fd = 0;
                    continue;
                }

                // Successful bind
                break;
            }
        }
        else
        {
            if ( ::bind ( _fd, res->ai_addr, res->ai_addrlen ) == SOCKET_ERROR )
            {
                exc = WinException ( WSAGetLastError(), format ( ERROR_NETWORK_PORT_BIND, address.port ), "" );
                LOG_SOCKET ( this, "%s", exc );
                closesocket ( _fd );
                _fd = 0;
                continue;
            }

            if ( isTCP() && ( ::listen ( _fd, SOMAXCONN ) == SOCKET_ERROR ) )
            {
                exc = WinException ( WSAGetLastError(), "listen failed", ERROR_NETWORK_GENERIC );
                LOG_SOCKET ( this, "%s", exc );
                closesocket ( _fd );
                _fd = 0;
            }

            // Successful bind
            break;
        }
    }

    if ( _fd == 0 )
    {
        LOG_SOCKET ( this, "%s; init failed", exc );
        throw exc;
    }

    // Update the address to resolve hostname
    if ( !address.empty() && _state != State::Listening )
    {
        if ( isTCP() && res )
        {
            address.addr = getAddrFromSockAddr ( res->ai_addr );
            address.invalidate();
        }
        else
        {
            addrInfo = address.getAddrInfo();
            address.addr = getAddrFromSockAddr ( addrInfo->ai_addr );
            address.invalidate();
        }
    }

    // Update the local port if bound to any available port
    if ( address.port == 0 )
    {
        sockaddr_storage sas;
        int saLen = sizeof ( sas );

        if ( getsockname ( _fd, ( sockaddr * ) &sas, &saLen ) == SOCKET_ERROR )
        {
            LOG_SOCKET ( this, "getsockname failed" );
            closesocket ( _fd );
            _fd = 0;
            THROW_WIN_EXCEPTION ( WSAGetLastError(), "getsockname failed", ERROR_NETWORK_GENERIC );
        }

        address.port = getPortFromSockAddr ( ( sockaddr * ) &sas );
        address.invalidate();
    }
}

bool Socket::send ( const char *buffer, size_t len )
{
    if ( _fd == 0 || isDisconnected() )
    {
        LOG_SOCKET ( this, "Cannot send over disconnected socket" );
        return false;
    }

    ASSERT ( isClient() == true );
    ASSERT ( _fd != 0 );
    ASSERT ( address.addr.empty() == false );

    size_t totalBytes = 0;

    while ( totalBytes < len || len == 0 )
    {
        int sentBytes = SOCKET_ERROR;

        if ( isTCP() )
        {
            LOG_SOCKET ( this, "send ( [ %u bytes ] )", len );
            sentBytes = ::send ( _fd, buffer, len, 0 );
        }
        else
        {
            LOG_SOCKET ( this, "sendto ( [ %u bytes ], '%s' )", len, address );
            sentBytes = ::sendto ( _fd, buffer, len, 0,
                                   address.getAddrInfo()->ai_addr, address.getAddrInfo()->ai_addrlen );
        }

        if ( sentBytes == SOCKET_ERROR )
        {
            const string err = WinException::getLastSocketError();

            // Disconnect the socket if an error occurred during send
            if ( isTCP() )
            {
                LOG_SOCKET ( this, "%s; send failed", err );
                LOG_SOCKET ( this, "disconnect due to send error" );
                socketDisconnected();
            }
            else
            {
                LOG_SOCKET ( this, "%s; sendto failed", err );
                disconnect();
            }

            return false;
        }

        if ( len == 0 )
            break;

        totalBytes += sentBytes;
    }

    return true;
}

bool Socket::send ( const char *buffer, size_t len, const IpAddrPort& address )
{
    if ( _fd == 0 || isDisconnected() )
    {
        LOG_SOCKET ( this, "Cannot send over disconnected socket" );
        return false;
    }

    ASSERT ( isUDP() == true );
    ASSERT ( _fd != 0 );
    ASSERT ( address.addr.empty() == false );

    size_t totalBytes = 0;

    while ( totalBytes < len || len == 0 )
    {
        LOG_SOCKET ( this, "sendto ( [ %u bytes ], '%s' )", len, address );
        int sentBytes = ::sendto ( _fd, buffer, len, 0,
                                   address.getAddrInfo()->ai_addr, address.getAddrInfo()->ai_addrlen );

        if ( sentBytes == SOCKET_ERROR )
        {
            LOG_SOCKET ( this, "%s; sendto failed", WinException::getLastSocketError() );
            return false;
        }

        if ( len == 0 )
            break;

        totalBytes += sentBytes;
    }

    return true;
}

int Socket::recv ( char *buffer, size_t& len )
{
    ASSERT ( isClient() == true );
    ASSERT ( isTCP() == true );
    ASSERT ( _fd != 0 );

    int recvBytes = ::recv ( _fd, buffer, len, 0 );

    if ( recvBytes == 0 )
        return WSAECONNRESET;

    if ( recvBytes == SOCKET_ERROR )
        return WSAGetLastError();

    len = recvBytes;
    return 0;
}

int Socket::recvfrom ( char *buffer, size_t& len, IpAddrPort& address )
{
    ASSERT ( isUDP() == true );
    ASSERT ( _fd != 0 );

    sockaddr_storage sas;
    int saLen = sizeof ( sas );

    int recvBytes = ::recvfrom ( _fd, buffer, len, 0, ( sockaddr * ) &sas, &saLen );

    if ( recvBytes == SOCKET_ERROR )
        return WSAGetLastError();

    len = recvBytes;
    address = ( sockaddr * ) &sas;
    return 0;
}

void Socket::resetBuffer()
{
    _readBuffer.reserve ( READ_BUFFER_SIZE );
    _readBuffer.resize ( READ_BUFFER_SIZE, ( char ) 0 );
    _readPos = 0;
}

void Socket::freeBuffer()
{
    _readBuffer.clear();
    _readBuffer.shrink_to_fit();
    _readPos = 0;
}

void Socket::consumeBuffer ( size_t bytes )
{
    if ( bytes == 0 )
        return;

    // Erase the consumed bytes (shifting the array)
    ASSERT ( bytes <= _readPos );
    _readBuffer.erase ( 0, bytes );
    _readBuffer.reserve ( READ_BUFFER_SIZE );
    _readBuffer.resize ( READ_BUFFER_SIZE, ( char ) 0 );
    _readPos -= bytes;
}

void Socket::socketRead()
{
    ASSERT ( _readPos < _readBuffer.size() );

    char *bufferStart = &_readBuffer[_readPos];
    size_t bufferLen = _readBuffer.size() - _readPos;

    IpAddrPort address = getRemoteAddress();
    int error = 0;

    if ( isTCP() )
        error = Socket::recv ( bufferStart, bufferLen );
    else
        error = Socket::recvfrom ( bufferStart, bufferLen, address );

    if ( error )
    {
        LOG_SOCKET ( this, "[%d] %s; %s failed",
                     error, WinException::getAsString ( error ), ( isTCP() ? "recv" : "recvfrom" ) );

        // Skip blocking reads
        if ( error == WSAEWOULDBLOCK )
            return;

        // WSAECONNRESET does not mean the UDP socket is dead, it just means Windows is reporting:
        // http://en.wikipedia.org/wiki/Internet_Control_Message_Protocol#Destination_unreachable
        if ( isUDP() && error == WSAECONNRESET )
            return;

        // Disconnect the socket if an error occurred during read
        LOG_SOCKET ( this, "disconnect due to read error" );

        if ( isTCP() )
            socketDisconnected();
        else
            disconnect();
        return;
    }

// #ifndef RELEASE
//     // Simulated packet loss
//     if ( rand() % 100 < _packetLoss )
//     {
//         LOG ( "Discarding [ %u bytes ] from '%s'", bufferLen, address );
//         return;
//     }
// #endif

    // Raw read mode
    if ( _isRaw )
    {
        LOG ( "Read [ %u bytes ] from '%s'", bufferLen, address );

        if ( owner )
            owner->socketRead ( this, bufferStart, bufferLen, address );
        return;
    }

    // Increment the buffer position
    _readPos += bufferLen;
    LOG ( "Read [ %u bytes ] from '%s'; %u bytes remaining in buffer", bufferLen, address, _readPos );

    // Handle zero byte packets
    if ( bufferLen == 0 )
    {
        LOG ( "Decoded 'NullMsg' using [ 0 bytes ]" );
        socketRead ( NullMsg, address );
        return;
    }

    if ( bufferLen <= 256 )
        LOG ( "Hex: %s", formatAsHex ( bufferStart, bufferLen ) );

    // Check if the first byte is a valid message type
    if ( _readPos >= sizeof ( MsgType ) && ! ::Protocol::checkMsgType ( * ( MsgType * ) &_readBuffer[0] ) )
    {
        LOG ( "Clearing invalid buffer!" );
        resetBuffer();
        return;
    }

    _gotGoodRead = true;

    // Try to decode as many messages from the buffer as possible
    for ( ;; )
    {
        size_t consumedBytes = 0;
        MsgPtr msg = ::Protocol::decode ( &_readBuffer[0], _readPos, consumedBytes );
        consumeBuffer ( consumedBytes );

        // Abort if a message could not be decoded
        if ( ! msg.get() )
            return;

        LOG ( "Decoded '%s' using [ %u bytes ]; %u bytes remaining in buffer", msg, consumedBytes, _readPos );
        socketRead ( msg, address );

        // Abort if the socket is de-allocated
        if ( ! SocketManager::get().isAllocated ( this ) )
            return;

        // Abort if socket is disconnected
        if ( isDisconnected() )
            return;
    }
}

MsgPtr Socket::share ( int processId )
{
    shared_ptr<WSAPROTOCOL_INFO> info ( new WSAPROTOCOL_INFO() );

    if ( WSADuplicateSocket ( _fd, processId, info.get() ) )
        THROW_WIN_EXCEPTION ( WSAGetLastError(), "WSADuplicateSocket failed", ERROR_NETWORK_GENERIC );

    // Workaround for Wine, because apparently these aren't set
    if ( isTCP() )
    {
        info->iSocketType = SOCK_STREAM;
        info->iProtocol = IPPROTO_TCP;
    }
    else
    {
        info->iSocketType = SOCK_DGRAM;
        info->iProtocol = IPPROTO_UDP;
    }

    SocketManager::get().remove ( this );

    LOG ( "Sharing:" );
    LOG ( "address='%s'; protocol=%s; state=%s", address, protocol, _state );

    return MsgPtr ( new SocketShareData ( address, protocol, _readBuffer, _readPos, _state, info ) );
}

SocketShareData::SocketShareData ( const IpAddrPort& address,
                                   Socket::Protocol protocol,
                                   const string& readBuffer,
                                   size_t readPos,
                                   Socket::State state,
                                   const shared_ptr<WSAPROTOCOL_INFO>& info )
    : address ( address )
    , protocol ( protocol )
    , readBuffer ( readBuffer )
    , readPos ( readPos )
    , state ( state )
    , info ( info ) {}

void SocketShareData::save ( cereal::BinaryOutputArchive& ar ) const
{
    ar ( address, protocol, readBuffer, readPos, isRaw, state, connectTimeout,
         info->dwServiceFlags1,
         info->dwServiceFlags2,
         info->dwServiceFlags3,
         info->dwServiceFlags4,
         info->dwProviderFlags,
         info->ProviderId.Data1,
         info->ProviderId.Data2,
         info->ProviderId.Data3,
         info->ProviderId.Data4,
         info->dwCatalogEntryId,
         info->ProtocolChain.ChainLen,
         info->ProtocolChain.ChainEntries,
         info->iVersion,
         info->iAddressFamily,
         info->iMaxSockAddr,
         info->iMinSockAddr,
         info->iSocketType,
         info->iProtocol,
         info->iProtocolMaxOffset,
         info->iNetworkByteOrder,
         info->iSecurityScheme,
         info->dwMessageSize,
         info->dwProviderReserved,
         info->szProtocol );

    ar ( udpType, Protocol::encode ( gbnState ), childSockets );
}

void SocketShareData::load ( cereal::BinaryInputArchive& ar )
{
    info.reset ( new WSAPROTOCOL_INFO() );

    ar ( address, protocol, readBuffer, readPos, isRaw, state, connectTimeout,
         info->dwServiceFlags1,
         info->dwServiceFlags2,
         info->dwServiceFlags3,
         info->dwServiceFlags4,
         info->dwProviderFlags,
         info->ProviderId.Data1,
         info->ProviderId.Data2,
         info->ProviderId.Data3,
         info->ProviderId.Data4,
         info->dwCatalogEntryId,
         info->ProtocolChain.ChainLen,
         info->ProtocolChain.ChainEntries,
         info->iVersion,
         info->iAddressFamily,
         info->iMaxSockAddr,
         info->iMinSockAddr,
         info->iSocketType,
         info->iProtocol,
         info->iProtocolMaxOffset,
         info->iNetworkByteOrder,
         info->iSecurityScheme,
         info->dwMessageSize,
         info->dwProviderReserved,
         info->szProtocol );

    string buffer;
    ar ( udpType, buffer, childSockets );

    size_t consumed;
    gbnState = Protocol::decode ( &buffer[0], buffer.size(), consumed );

    ASSERT ( consumed == buffer.size() );
}

SocketPtr Socket::shared ( Socket::Owner *owner, const SocketShareData& data )
{
    if ( data.isTCP() )
        return TcpSocket::shared ( owner, data );
    else
        return UdpSocket::shared ( owner, data );
}

TcpSocket& Socket::getAsTCP()
{
    ASSERT ( typeid ( *this ) == typeid ( TcpSocket ) );
    return *static_cast<TcpSocket *> ( this );
}

const TcpSocket& Socket::getAsTCP() const
{
    ASSERT ( typeid ( *this ) == typeid ( TcpSocket ) );
    return *static_cast<const TcpSocket *> ( this );
}

UdpSocket& Socket::getAsUDP()
{
    ASSERT ( typeid ( *this ) == typeid ( UdpSocket ) );
    return *static_cast<UdpSocket *> ( this );
}

const UdpSocket& Socket::getAsUDP() const
{
    ASSERT ( typeid ( *this ) == typeid ( UdpSocket ) );
    return *static_cast<const UdpSocket *> ( this );
}

SmartSocket& Socket::getAsSmart()
{
    ASSERT ( typeid ( *this ) == typeid ( SmartSocket ) );
    return *static_cast<SmartSocket *> ( this );
}

const SmartSocket& Socket::getAsSmart() const
{
    ASSERT ( typeid ( *this ) == typeid ( SmartSocket ) );
    return *static_cast<const SmartSocket *> ( this );
}

void Socket::forceReusePort ( bool enable )
{
    enableForceReusePort = enable;
}

void Socket::setPacketLoss ( uint8_t percentage )
{
    _packetLoss = percentage;
}

void Socket::setCheckSumFail ( uint8_t percentage )
{
    _hashFailRate = percentage;
}

