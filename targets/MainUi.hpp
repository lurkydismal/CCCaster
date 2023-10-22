#pragma once

#include "Messages.hpp"
#include "IpAddrPort.hpp"
#include "Controller.hpp"
#include "ControllerManager.hpp"
#include "KeyValueStore.hpp"
// #include "MainUpdater.hpp"
#include "Lobby.hpp"
#include "MatchmakingManager.hpp"

#include <string>
#include <memory>


// The function to run the game with the provided options
typedef void ( * RunFuncPtr ) ( const IpAddrPort& address, const Serializable& config );


// Function that computes the delay from the latency
inline int computeDelay ( double latency )
{
    return ( int ) ceil ( latency / ( 1000.0 / 60 ) );
}


class ConsoleUi;

class MainUi
    : private Controller::Owner
    , private ControllerManager::Owner
    , private Lobby::Owner
    , private MatchmakingManager::Owner
{
public:

    InitialConfig initialConfig;

    std::string sessionMessage;

    std::string sessionError;

    std::vector<std::string> lobbyText;
    std::vector<std::string> lobbyIps;
    std::vector<std::string> lobbyIds;

    MainUi();

    void initialize();

    void main ( RunFuncPtr run );

    void display ( const std::string& message, bool replace = true );

    bool connected ( const InitialConfig& initialConfig, const PingStats& pingStats );

    void spectate ( const SpectateConfig& spectateConfig );

    bool confirm ( const std::string& question );


    void setMaxRealDelay ( uint8_t delay );

    void setDefaultRollback ( uint8_t rollback );

    const KeyValueStore& getConfig() const { return _config; }

    const NetplayConfig& getNetplayConfig() const { return _netplayConfig; }


    static void *getConsoleWindow();

    static std::string formatStats ( const PingStats& pingStats );

    bool isServer() { return serverMode; }
    void hostReady();
    void sendConnected();

private:

    std::shared_ptr<ConsoleUi> _ui;

    std::shared_ptr<Lobby> _lobby;

    std::shared_ptr<MatchmakingManager> _mmm;

    KeyValueStore _config;

    IpAddrPort _address;

    NetplayConfig _netplayConfig;

    Controller *_currentController = 0;

    uint32_t _mappedKey = 0;

    bool _upToDate = false;

    bool serverMode = false;

    bool isMatchmaking = false;

    void netplay ( RunFuncPtr run );
    void spectate ( RunFuncPtr run );
    void broadcast ( RunFuncPtr run );
    void offline ( RunFuncPtr run );
    void controls();
    void settings();
    void results();
    void wait();

    bool areYouSure();

    bool gameMode ( bool below );
    bool offlineGameMode();

    void controllerKeyMapped ( Controller *controller, uint32_t key ) override;

    void joystickAttached ( Controller *controller ) override {};
    void joystickToBeDetached ( Controller *controller ) override;

    void saveConfig();
    void loadConfig();

    void saveMappings ( const Controller& controller );
    void loadMappings ( Controller& controller );

    void alertUser();

    std::string formatPlayer ( const SpectateConfig& spectateConfig, uint8_t player ) const;

    bool configure ( const PingStats& pingStats );

    std::string getUpdate ( bool isStartup = false );

    void connectionFailed ( Lobby *lobby );
    void unlock ( Lobby *lobby );

    void connectionFailed( MatchmakingManager* lobby );
    void setAddr( MatchmakingManager* lobby, std::string addr );
    void setMode( MatchmakingManager* lobby, std::string mode );
    void unlock( MatchmakingManager* lobby );

};
