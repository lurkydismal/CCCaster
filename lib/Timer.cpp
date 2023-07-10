#include "Timer.hpp"
#include "TimerManager.hpp"

using namespace std;

Timer::Timer( Owner* owner ) : owner( owner ) {
    TimerManager::get().add( this );
}

Timer::~Timer() {
    TimerManager::get().remove( this );
}

void Timer::start( uint64_t delay ) {
    _delay = delay;
}

void Timer::stop() {
    _delay = _expiry = 0;
}
