use std::sync::{Arc, Mutex};

use esp_idf_hal::{i2c::{I2cDriver}, delay::{FreeRtos, BLOCK}};
use esp_idf_sys::EspError;

use crate::{joystick::{Direction}, lora::{LoRaStatus, self}};
use crate::common::ControlData;


const LCD_DRIVER_I2C_ADDR: u8 = 0x27;


#[repr(u8)]
#[derive(Debug, Clone, Copy)]
pub enum LineNumber {
    FirstLine = 0x80|0x00,
    SecondLine = 0x80|0x40,
    ThirdLine = 0x80|0x14,
    FourthLine = 0x80|0x54
}

pub enum DisplayMode {
    Welcome,
    Data,
}


pub struct LCD {
    content: String,
    line: LineNumber,
    index: u8
}

impl LCD {
    pub fn new() -> LCD {
        return LCD { content: String::from(""), line: LineNumber::FirstLine, index: 0 };
    }

    pub fn init_lcd(&mut self, i2c: &mut I2cDriver) -> Result<(), EspError> {
        FreeRtos::delay_ms(50);
        self.send_cmd(i2c, 0x30)?;
        FreeRtos::delay_ms(5);
        self.send_cmd(i2c, 0x30)?;
        FreeRtos::delay_ms(1);
        self.send_cmd(i2c, 0x30)?;
        FreeRtos::delay_ms(10);
        self.send_cmd(i2c, 0x20)?;
        FreeRtos::delay_ms(10);

        FreeRtos::delay_ms(50);
        self.send_cmd(i2c, 0x28)?;
        FreeRtos::delay_ms(5);
        self.send_cmd(i2c, 0x08)?;
        FreeRtos::delay_ms(1);
        self.send_cmd(i2c, 0x01)?;
        FreeRtos::delay_ms(10);
        self.send_cmd(i2c, 0x06)?;
        FreeRtos::delay_ms(10);
        self.send_cmd(i2c, 0x0c)?;
        FreeRtos::delay_ms(10); 


        Ok(())
    }


    pub fn lcd_thread(&mut self, data_ptr: Arc<Mutex<ControlData>>, i2c_ptr: Arc<Mutex<I2cDriver>>) {
        let mut data = data_ptr.lock().unwrap();
        let mut i2c = i2c_ptr.lock().unwrap();

        self.draw_direction(&mut i2c, data.stick.direction);
        self.draw_throttle_percentage(&mut i2c, data.throttle.as_percentage().unwrap());

    }

    /// Draws to the LCD only when a data changes.
    pub fn draw_flight_data_template(&mut self, i2c: &mut I2cDriver) -> Result<(), EspError> {
        self.set_cursor(i2c, LineNumber::FirstLine, 0);
        self.send_string(i2c, String::from("THR:"));

        self.set_cursor(i2c, LineNumber::FirstLine, 14);
        self.send_string(i2c, String::from("Dir:"));

        self.set_cursor(i2c, LineNumber::ThirdLine, 0);
        self.send_string(i2c, String::from("Temp:"));

        self.set_cursor(i2c, LineNumber::FourthLine, 0);
        self.send_string(i2c, String::from("Pressure:"));

        self.draw_direction(i2c, Direction::NorthEast);

        Ok(())
    }

    pub fn draw_throttle_percentage(&mut self, i2c: &mut I2cDriver, thr_percentage: u8) -> Result<(), EspError> {
        self.set_cursor(i2c, LineNumber::FirstLine, 4)?;

        let mut thr_percentage_conversion: String = if thr_percentage < 10 {
            format!("{}{}{}", " ", thr_percentage , "%")
        } else {
            format!("{}{}", thr_percentage , "%")
        };

        self.send_string(i2c, thr_percentage_conversion)?;

        Ok(())
    }

    pub fn draw_lora_status(&mut self, i2c: &mut I2cDriver, lora_status: LoRaStatus) -> Result<(), EspError> {
        self.set_cursor(i2c, LineNumber::SecondLine, 5)?;

        self.send_string(i2c, lora_status.as_str().to_string())?;

        Ok(())
    }

    pub fn draw_direction(&mut self, i2c: &mut I2cDriver, direction: Direction) -> Result<(), EspError> {
        self.set_cursor(i2c, LineNumber::FirstLine, 18)?;

        let mut direction_conversion = if direction.as_str().len() < 2 {
            format!("{}{}", " ", direction.as_str())
        } else {
            direction.as_str().to_string()
        };

        self.send_string(i2c, direction_conversion)?;

        Ok(())
    }

    pub fn draw_temperature(&mut self, i2c: &mut I2cDriver, temp_in_c: i8) -> Result<(), EspError> {
        self.set_cursor(i2c, LineNumber::ThirdLine, 5)?;

        let temp_conversion = if temp_in_c < 10 && temp_in_c >= 0 {
            format!("{}{}C", " ", temp_in_c)
        } else {
            format!("{}C", temp_in_c)
        };

        self.send_string(i2c, temp_conversion)?;

        Ok(())
    }

    pub fn draw_pressure(&mut self, i2c: &mut I2cDriver, pressure_in_hpa: u16) -> Result<(), EspError> {
        self.set_cursor(i2c, LineNumber::FourthLine, 4)?;

        let prs_conversion = if pressure_in_hpa.to_string().len() < 4 {
            format!("{}{}{}", " ", pressure_in_hpa, "hPa")
        } else {
            format!("{}{}", pressure_in_hpa, "hPa")
        };

        self.send_string(i2c, prs_conversion)?;

        Ok(())
    }


    fn send_cmd(&mut self, i2c: &mut I2cDriver,  cmd: u8) -> Result<(), EspError> {
        let mut data_u: u8;
        let mut data_l: u8;
        let mut data: [u8; 4] = [0, 0, 0, 0];

        data_u = cmd & 0xf0;
        data_l = (cmd << 4) & 0xf0;

        data[0] = data_u|0x0C;  //en=1, rs=0
        data[1] = data_u|0x08;  //en=0, rs=0
        data[2] = data_l|0x0C;  //en=1, rs=0
        data[3] = data_l|0x08;  //en=0, rs=0

        i2c.write(LCD_DRIVER_I2C_ADDR, &data, BLOCK)?;
        FreeRtos::delay_ms(1);
        Ok(())

    }

    fn send_data(&mut self, i2c: &mut I2cDriver, data_unit: u8) -> Result<(), EspError> {
        let mut data: [u8; 4] = [0, 0, 0, 0];

        let data_u: u8 = data_unit & 0xf0;
        let data_l: u8 = (data_unit << 4) & 0xf0;

        data[0] = data_u|0x0D;  //en=1, rs=1
        data[1] = data_u|0x09;  //en=0, rs=1
        data[2] = data_l|0x0D;  //en=1, rs=1
        data[3] = data_l|0x09;  //en=0, rs=1

        i2c.write(LCD_DRIVER_I2C_ADDR, &data, BLOCK)?;
        FreeRtos::delay_ms(1);
        Ok(())
    }

    fn send_string(&mut self,  i2c: &mut I2cDriver, str: String) -> Result<(), EspError> {
        for chr in str.chars() {
            self.send_data(i2c, chr as u8)?;
        }

        Ok(())
    }

    fn clear_screen(&mut self, i2c: &mut I2cDriver) -> Result<(), EspError> {
        self.send_cmd(i2c, 0x01)?;
        Ok(())
    }

    fn select_line(&mut self, i2c: &mut I2cDriver, line_num: LineNumber) -> Result<(), EspError>  {
        self.send_cmd(i2c, line_num as u8)?;
        Ok(())
    }

    fn set_cursor(&mut self, i2c: &mut I2cDriver, line_num: LineNumber, mut index: u8) -> Result<(), EspError> {
        if index > 19 {
            index = 19;
        }

        self.line = line_num.clone();
        self.index = index;

        match line_num {
            LineNumber::FirstLine => self.send_cmd(i2c, 0x80|(0x00+index)),
            LineNumber::SecondLine => self.send_cmd(i2c, 0x80|(0x40+index)),
            LineNumber::ThirdLine => self.send_cmd(i2c, 0x80|(0x14+index)),
            LineNumber::FourthLine => self.send_cmd(i2c, 0x80|(0x54+index)),
        }
    }

    
}
