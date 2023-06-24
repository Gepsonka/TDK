use core::time;
use std::thread;


pub mod network;



fn main() {
    loop {
        thread::sleep(time::Duration::from_secs(1));
    }
}
