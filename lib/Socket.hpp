#pragma once

#include <memory>
#include <vector>

#include "Enum.hpp"
#include "GoBackN.hpp"
#include "IpAddrPort.hpp"

#define DEFAULT_CONNECT_TIMEOUT ( 5000 )

#define LOG_SOCKET( SOCKET, FORMAT, ... )                                      \
    LOG( "%s socket=%08x; fd=%08x; state=%s; address='%s'; isRaw=%u; " FORMAT, \
         SOCKET->protocol, SOCKET, SOCKET->_fd, SOCKET->_state,                \
         SOCKET->address, SOCKET->_isRaw, ##__VA_ARGS__ )

// Forward declarations
struct _WSAPROTOCOL_INFOA;
typedef struct _WSAPROTOCOL_INFOA WSAPROTOCOL_INFO;
class Socket;
class TcpSocket;
class UdpSocket;
class SmartSocket;
struct SocketShareData;

typedef std::shared_ptr< Socket > SocketPtr;

// Generic socket base class
class Socket {
public:
    // Socket owner interface
    struct Owner {
        // Accepted a socket from server socket
        virtual void socketAccepted( Socket* serverSocket ) = 0;

        // Socket connected event
        virtual void socketConnected( Socket* socket ) = 0;

        // Socket disconnected event
        virtual void socketDisconnected( Socket* socket ) = 0;

        // Socket protocol message read event (only called if NOT isRaw)
        virtual void socketRead( Socket* socket,
                                 const MsgPtr& msg,
                                 const IpAddrPort& address ) = 0;

        // Socket raw data read event (only called if isRaw)
        virtual void socketRead( Socket* socket,
                                 const char* buffer,
                                 size_t len,
                                 const IpAddrPort& address ) {}
    };

    // Socket protocol
    ENUM( Protocol, TCP, UDP, Smart );

    // Connection state
    ENUM( State, Listening, Connecting, Connected, Disconnected );

    // Socket owner
    Owner* owner = 0;

    // Socket address
    // For server sockets, only the port should be set to the locally bound port
    // For client sockets, this is the remote server address
    IpAddrPort address;

    // Socket protocol
    const Protocol protocol;

    // Constructor
    Socket( Owner* owner,
            const IpAddrPort& address,
            Protocol protocol,
            bool isRaw );

    // Virtual destructor
    virtual ~Socket();

    // Completely disconnect the socket
    virtual void disconnect();

    // Socket state query functions
    bool isTCP() const { return ( protocol == Protocol::TCP ); }
    bool isUDP() const { return ( protocol == Protocol::UDP ); }
    bool isSmart() const { return ( protocol == Protocol::Smart ); }
    bool gotGoodRead() const { return _gotGoodRead; }
    virtual State getState() const { return _state; }
    virtual bool isConnecting() const {
        return isClient() && ( _state == State::Connecting );
    }
    virtual bool isConnected() const {
        return isClient() && ( _state == State::Connected );
    }
    virtual bool isDisconnected() const {
        return ( _state == State::Disconnected );
    }
    virtual bool isClient() const { return !address.addr.empty(); }
    virtual bool isServer() const { return address.addr.empty(); }
    virtual const IpAddrPort& getRemoteAddress() const {
        if ( isServer() )
            return NullAddress;
        return address;
    }

    // Send raw bytes directly, a return value of false indicates socket is
    // disconnected
    virtual bool send( const char* buffer, size_t len );
    virtual bool send( const char* buffer,
                       size_t len,
                       const IpAddrPort& address );

    // Accept a new socket, should not be called without an socketAccepted.
    // Check socket implementation for specific behaviours.
    virtual SocketPtr accept( Owner* owner ) = 0;

    // Get the data needed to share this socket with another process
    virtual MsgPtr share( int processId );

    // Send a protocol message, a return value of false indicates socket is
    // disconnected
    virtual bool send( SerializableMessage* message,
                       const IpAddrPort& address = NullAddress ) = 0;
    virtual bool send( SerializableSequence* message,
                       const IpAddrPort& address = NullAddress ) = 0;
    virtual bool send( const MsgPtr& message,
                       const IpAddrPort& address = NullAddress ) = 0;
    bool send( const Serializable& message,
               const IpAddrPort& address = NullAddress ) {
        return send(
            MsgPtr( const_cast< Serializable* >( &message ), ignoreMsgPtr ),
            address );
    }

    // Set the packet loss for testing purposes
    void setPacketLoss( uint8_t percentage );

    // Set the check sum fail percentage for testing purposes
    void setCheckSumFail( uint8_t percentage );

    // Cast this to another socket type
    TcpSocket& getAsTCP();
    const TcpSocket& getAsTCP() const;
    UdpSocket& getAsUDP();
    const UdpSocket& getAsUDP() const;
    SmartSocket& getAsSmart();
    const SmartSocket& getAsSmart() const;

    // Force reuse of existing ports
    static void forceReusePort( bool enable );

    // Create a socket from SocketShareData
    static SocketPtr shared( Socket::Owner* owner,
                             const SocketShareData& data );

    friend class SocketManager;
    friend class SmartSocket;

protected:
    // Socket read buffer
    std::string _readBuffer;

    // The position for the next read event.
    // In raw mode, this should be manually updated, otherwise each read will at
    // the same position. In message mode, this is automatically managed, and is
    // only reset when a decode fails.
    size_t _readPos = 0;

    // Raw socket type flag
    bool _isRaw = false;

    // Did we get a successful recv()? If not, it is possible we never really
    // connected
    bool _gotGoodRead = false;

    // Connection state
    State _state = State::Disconnected;

    // Underlying socket fd
    int _fd = 0;

    // Initial connect timeout
    uint64_t _connectTimeout = DEFAULT_CONNECT_TIMEOUT;

    // Packet loss percentage for testing purposes
    uint8_t _packetLoss = 0;

    // Hash failure percentage for testing purposes
    uint8_t _hashFailRate = 0;

    // Reset the read buffer to its initial size
    void resetBuffer();

    // Free the read buffer
    void freeBuffer();

    // Consume bytes from the front of the buffer
    void consumeBuffer( size_t bytes );

    // TCP event callbacks
    virtual void socketAccepted() {}
    virtual void socketConnected() {}
    virtual void socketDisconnected() {}

    // Read event callback, calls the function below if NOT isRaw
    virtual void socketRead();

    // Read protocol message callback, must be implemented, only called if NOT
    // isRaw
    virtual void socketRead( const MsgPtr& msg, const IpAddrPort& address ) = 0;

    // Initialize the socket fd with the provided address and protocol
    void init();

    // Read raw bytes directly, 0 on success, otherwise returns the socket error
    // code
    int recv( char* buffer, size_t& len );
    int recvfrom( char* buffer, size_t& len, IpAddrPort& address );
};

// Contains data for sharing a socket across processes
struct SocketShareData : public SerializableSequence {
    IpAddrPort address;
    Socket::Protocol protocol;
    std::string readBuffer;
    size_t readPos = 0;
    uint8_t isRaw = 0;
    Socket::State state;
    uint64_t connectTimeout = DEFAULT_CONNECT_TIMEOUT;
    std::shared_ptr< WSAPROTOCOL_INFO > info;

    // Extra data for UDP sockets
    uint8_t udpType = 0;
    MsgPtr gbnState;
    std::unordered_map< IpAddrPort, GoBackN > childSockets;

    SocketShareData( const IpAddrPort& address,
                     Socket::Protocol protocol,
                     const std::string& readBuffer,
                     size_t readPos,
                     Socket::State state,
                     const std::shared_ptr< WSAPROTOCOL_INFO >& info );

    bool isTCP() const { return ( protocol == Socket::Protocol::TCP ); }
    bool isUDP() const { return ( protocol == Socket::Protocol::UDP ); }

    DECLARE_MESSAGE_BOILERPLATE( SocketShareData )
};
