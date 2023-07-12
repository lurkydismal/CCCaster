#pragma once

#include <list>

#include "Protocol.hpp"
#include "Timer.hpp"

#define DEFAULT_SEND_INTERVAL ( 50 )

struct AckSequence : public SerializableSequence {
    AckSequence( uint32_t sequence ) : SerializableSequence( sequence ) {}

    EMPTY_MESSAGE_BOILERPLATE( AckSequence )
};

struct SplitMessage : public SerializableSequence {
    MsgType origMsgType;

    std::string bytes;

    uint32_t index, count;

    bool isLastMessage() const { return ( index + 1 == count ); }

    std::string str() const {
        std::stringstream ss;
        ss << origMsgType << "(" << ( index + 1 ) << "/" << count << ")";
        return ss.str();
    }

    SplitMessage( MsgType origMsgType,
                  const std::string& bytes,
                  uint32_t index = 0,
                  uint32_t count = 1 )
        : origMsgType( origMsgType ),
          bytes( bytes ),
          index( index ),
          count( count ) {}

    PROTOCOL_MESSAGE_BOILERPLATE( SplitMessage,
                                  origMsgType,
                                  index,
                                  count,
                                  bytes )
};

class GoBackN : public SerializableSequence, private Timer::Owner {
   public:
    struct Owner {
        // Send a message via raw socket
        virtual void goBackNSendRaw( GoBackN* gbn, const MsgPtr& msg ) = 0;

        // Receive a raw non-sequenced message
        virtual void goBackNRecvRaw( GoBackN* gbn, const MsgPtr& msg ) = 0;

        // Receive a message from GoBackN
        virtual void goBackNRecvMsg( GoBackN* gbn, const MsgPtr& msg ) = 0;

        // Timeout GoBackN if keep alive is enabled
        virtual void goBackNTimeout( GoBackN* gbn ) = 0;
    };

    Owner* owner = 0;

    // Constructors
    GoBackN( const GoBackN& other );
    GoBackN( Owner* owner,
             uint64_t interval = DEFAULT_SEND_INTERVAL,
             uint64_t timeout = 0 );
    GoBackN( Owner* owner, const GoBackN& state );
    GoBackN& operator=( const GoBackN& other );

    // Send a message via GoBackN
    void sendViaGoBackN( SerializableSequence* message );
    void sendViaGoBackN( const MsgPtr& msg );

    // Receive a message from the raw socket
    void recvFromSocket( const MsgPtr& msg );

    // Get / set the interval to send packets, should be non-zero
    uint64_t getSendInterval() const { return _interval; }
    void setSendInterval( uint64_t interval );

    // Get / set the timeout for keep alive packets, 0 to disable
    uint64_t getKeepAlive() const { return _keepAlive; }
    void setKeepAlive( uint64_t timeout );

    // Get the number of messages sent and received
    uint32_t getSendCount() const { return _sendSequence; }
    uint32_t getRecvCount() const { return _recvSequence; }

    // Get the number of messages ACKed
    uint32_t getAckCount() const { return _ackSequence; }

    // Delay sending the next keep alive packet
    void delayKeepAliveOnce();

    // Reset the state of GoBackN
    void reset();

    // Log the send the list for debugging purposes
    void logSendList() const;

    DECLARE_MESSAGE_BOILERPLATE( GoBackN )

   private:
    // Last sent and received sequences
    uint32_t _sendSequence = 0, _recvSequence = 0;

    // Last ACKed sequence
    uint32_t _ackSequence = 0;

    // Current list of messages to repeatedly send
    std::list< MsgPtr > _sendList;

    // Current position in the sendList
    std::list< MsgPtr >::const_iterator _sendListPos;

    // Timer for repeatedly sending messages
    TimerPtr _sendTimer;

    // Buffer for accumulating split messages
    std::string _recvBuffer;

    // The interval to send packets, should be non-zero
    uint64_t _interval = DEFAULT_SEND_INTERVAL;

    // The timeout for keep alive packets, 0 to disable
    uint64_t _keepAlive = 0;

    // The countdown timer for the keep alive packets
    uint32_t _countDown = 0;

    // Delay sending the keep alive packet for one iteration
    bool _skipNextKeepAlive = false;

    // Timer callback that sends the messages
    void timerExpired( Timer* timer ) override;

    // Start the timer if necessary
    void checkAndStartTimer();

    // Refresh keep alive count down
    void refreshKeepAlive();
};
