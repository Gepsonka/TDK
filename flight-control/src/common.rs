use esp_idf_hal::adc::{self, Atten11dB};
use esp_idf_hal::gpio::{Gpio32, Gpio26, Gpio25, Gpio33, Gpio27, PinDriver, Input};

use crate::altimeter::Altimeter;
use crate::joystick::Joystick;
use crate::{joystick::Direction, throttle::Throttle};
use crate::lora::{LoRaStatus, LoRa};
use std::sync::{Arc, Mutex};


pub trait PeripherialHandler {
    fn thread_event_loop(global_control: Arc<Mutex<ControlData>>);
}

pub struct ControlData <'a> {
    pub throttle: Throttle<'a, adc::ADC2>,
    pub stick: Joystick<'a, Gpio32, Gpio26, Gpio25, Gpio33>,
    pub lora: LoRa,
    pub altimeter: Altimeter,
}

impl <'a> ControlData<'a> {
    pub fn new(
        mut adc_driver: adc::AdcDriver<'a, adc::ADC2>, 
        mut adc_pin: esp_idf_hal::adc::AdcChannelDriver<'a, Gpio27, Atten11dB<adc::ADC2>>,
        stick_N_pin: PinDriver<'a, Gpio32, Input>,
        stick_W_pin: PinDriver<'a, Gpio26, Input>,
        stick_E_pin: PinDriver<'a, Gpio25, Input>,
        stick_S_pin: PinDriver<'a, Gpio33, Input>
    ) -> ControlData<'a> {
        ControlData { 
            throttle: Throttle::new(adc_driver, adc_pin),
            stick: Joystick::new(stick_N_pin, stick_W_pin, stick_E_pin, stick_S_pin),
            lora: LoRa::new(),
            altimeter: Altimeter::new() }
    }
}