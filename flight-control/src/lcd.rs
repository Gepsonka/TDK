use esp_idf_hal::{i2c::{I2cDriver}, delay::{FreeRtos, BLOCK}};
use esp_idf_sys::EspError;

use crate::joystick::{Direction};
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

    pub fn send_cmd(&mut self, i2c: &mut I2cDriver,  cmd: u8) -> Result<(), EspError> {
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

    pub fn send_data(&mut self, i2c: &mut I2cDriver, data_unit: u8) -> Result<(), EspError> {
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

    pub fn send_string(&mut self,  i2c: &mut I2cDriver, str: String) -> Result<(), EspError> {
        for chr in str.chars() {
            self.send_data(i2c, chr as u8)?;
        }

        Ok(())
    }

    pub fn clear_screen(&mut self, i2c: &mut I2cDriver) -> Result<(), EspError> {
        self.send_cmd(i2c, 0x01)?;
        Ok(())
    }

    pub fn select_line(&mut self, i2c: &mut I2cDriver, line_num: LineNumber) -> Result<(), EspError>  {
        self.send_cmd(i2c, line_num as u8)?;
        Ok(())
    }

    pub fn set_cursor(&mut self, i2c: &mut I2cDriver, line_num: LineNumber, mut index: u8) -> Result<(), EspError> {
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

    pub fn draw_data(&mut self, i2c: &mut I2cDriver, data: ControlData) -> Result<(), EspError> {
        self.set_cursor(i2c, LineNumber::FirstLine, 0)?;
        self.send_string(i2c, String::from("THR: 5%"))?;
        self.set_cursor(i2c, LineNumber::FirstLine, 14)?;
        self.send_string(i2c, String::from("DIR:SW"))?;
        self.set_cursor(i2c, LineNumber::SecondLine, 0)?;
        self.send_string(i2c, String::from("LoRa:Online"))?;
        self.set_cursor(i2c, LineNumber::ThirdLine, 0)?;
        self.send_string(i2c, String::from("Temp: 5C"))?;
        self.set_cursor(i2c, LineNumber::FourthLine, 0)?;
        self.send_string(i2c, String::from("PRS:1029hpa"))?;

        Ok(())
    }
}