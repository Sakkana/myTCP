#ifndef SPONGE_LIBSPONGE_TIMER_HH
#define SPONGE_LIBSPONGE_TIMER_HH

#include "tcp_config.hh"

#include <iostream>

class Timer {
public:
    Timer(const uint16_t retx_timeout);

    //! double the retransmission timeout
    void double_RTO();
    
    //! reset and start the timer
    //  after double the RTO 
    void restart();
    
    // Set the RTO back to its initial value
    void reset_to_intial();
    
    //! is timer running ?
    bool is_running();

    //! is the timer espired ?
    bool is_expired();
    
    //! get the rest time before timeout
    int get_alarm();
    
    //! stop the timer
    void stop();
    
    //! set the alarm
    void set_alarm(const size_t ms_since_last_tick);

private:
    //! initial retransmission timeout
    unsigned int _initial_RTO;
    //! current retransmission timeout in use
    unsigned int _current_RTO;
    //! alarm that record the elapse of the time
    int _alarm{0};
    //! whether the timer is running
    bool _is_running{false};
};

#endif  // SPONGE_LIBSPONGE_TIMER_HH