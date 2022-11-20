#include "timer.hh"

// Timer class for retransmission;
// If no acknowledgment has been received 
// for the data in a given segment before 
// the timer expires, 
// then the segment is retransmitted.


Timer::Timer(const uint16_t retx_timeout)
        : _initial_RTO(retx_timeout)
        , _current_RTO(retx_timeout)
        {}

//! exponential backof
// If tick is called 
// and the retransmission timer has expired
// If the window size is nonzero
void Timer::double_RTO() {
    _current_RTO += _current_RTO;
}

// Reset the retransmission timer 
// and start it such that it expires 
// after RTO milliseconds 
// (taking into account 
// that you may have just doubled the value of RTO!).
void Timer::restart() {
    _alarm = static_cast<int>(_current_RTO);
    _is_running = true;
}

// when the receiver gives the sender an ackno 
// that acknowledges the successful receipt of new data
void Timer::reset_to_intial() {
    _current_RTO = _initial_RTO;
}

//! stop the timer
void Timer::stop() {
    _is_running = false;
}

//! set the alarm when "tick()" is called
void Timer::set_alarm(const size_t ms_since_last_tick) {
    _alarm -= static_cast<int>(ms_since_last_tick);
}

//! is the timer expired ?
bool Timer::is_expired() {
    return _alarm <= 0;
}

bool Timer::is_running() { return _is_running; }

int Timer::get_alarm() { return _alarm; }

