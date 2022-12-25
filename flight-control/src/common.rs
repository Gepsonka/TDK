use crate::joystick::Direction;
use crate::lora::LoRaStatus;
use std::sync::{Arc, Mutex};


pub trait PeripherialHandler {
    fn thread_event_loop(global_control: Arc<Mutex<ControlData>>);
}

pub struct ControlData {
    throttle: u8,
    direction: Direction,
    lora_status: LoRaStatus,
    temperature: i8,
    pressure: i16
}

impl ControlData {
    pub fn new() -> ControlData {
        ControlData { throttle: 0, direction: Direction::North, lora_status: LoRaStatus::Offline, temperature: 0, pressure: 1000 }
    }
}