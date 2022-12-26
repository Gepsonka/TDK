use core::time;
use std::{sync::{Arc, Mutex}, thread};

use esp_idf_hal::gpio::{PinDriver, Gpio32, Input, Gpio34, Gpio35, Gpio33};
use esp_idf_sys::EspError;
use esp_idf_hal::gpio::Pin;

use crate::common::{PeripherialHandler, ControlData};




#[derive(Clone, Copy)]
pub enum Direction {
    Neutral,
    North,
    NorthEast,
    NorthWest,
    South,
    SouthEast,
    SouthWest,
    East,
    West,
}

impl Direction {
    pub fn as_str(&self) -> &'static str {
        match self {
            Direction::Neutral => "Ne",
            Direction::North => "N",
            Direction::NorthEast => "NE",
            Direction::NorthWest => "NW",
            Direction::South => "S",
            Direction::SouthEast => "SE",
            Direction::SouthWest => "SW",
            Direction::East => "E",
            Direction::West => "W"
        }
    }
}

pub struct Joystick<'a, GpioN, GpioW, GpioE, GpioS>
where GpioN: Pin,
      GpioW: Pin,
      GpioE: Pin,
      GpioS: Pin
{
    N_pin: PinDriver<'a, GpioN, Input>,
    W_pin: PinDriver<'a, GpioW, Input>,
    E_pin: PinDriver<'a, GpioE, Input>,
    S_pin: PinDriver<'a, GpioS, Input>,
    direction: Direction
}


impl <'a, GpioN, GpioW, GpioE, GpioS> Joystick<'a, GpioN, GpioW, GpioE, GpioS> 
where GpioN: Pin,
      GpioW: Pin,
      GpioE: Pin,
      GpioS: Pin
{
    pub fn new(
        N_pin: PinDriver<'a, GpioN, Input>,
        W_pin: PinDriver<'a, GpioW, Input>,
        E_pin: PinDriver<'a, GpioE, Input>,
        S_pin: PinDriver<'a, GpioS, Input>
    ) -> Joystick<'a, GpioN, GpioW, GpioE, GpioS> {
        Joystick { N_pin, W_pin, E_pin, S_pin, direction: Direction::Neutral }
    }

    pub fn read_direction(&mut self) -> Result<Direction, EspError> {
        let mut dir: Direction;

        if self.N_pin.is_high() {
            if self.W_pin.is_high() && self.E_pin.is_low() {
                self.direction = Direction::NorthWest;
            } else if self.E_pin.is_high() && self.W_pin.is_low() {
                self.direction = Direction::NorthEast;
            } else {
                self.direction = Direction::North;
            }

        } else if self.W_pin.is_high() {
            if self.N_pin.is_high() && self.S_pin.is_low() {
                self.direction = Direction::NorthWest;
            } else if self.S_pin.is_high() && self.N_pin.is_low() {
                self.direction = Direction::SouthWest;
            } else {
                self.direction = Direction::West;
            }

        } else if self.E_pin.is_high() {
            if self.N_pin.is_high() && self.S_pin.is_low() {
                self.direction = Direction::NorthEast;
            } else if self.S_pin.is_high() && self.N_pin.is_low() {
                self.direction = Direction::SouthEast;
            } else {
                self.direction = Direction::East;
            }

        } else if self.S_pin.is_high() {
            if self.W_pin.is_high() && self.E_pin.is_low() {
                self.direction = Direction::SouthWest;
            } else if self.E_pin.is_high() && self.W_pin.is_low() {
                self.direction = Direction::SouthEast;
            } else {
                self.direction = Direction::South;
            }
        } else {
            self.direction = Direction::Neutral;
        }

        Ok(self.direction.clone())

    }    


}
