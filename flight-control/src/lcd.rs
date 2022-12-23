use esp_idf_hal::{i2c::{I2cDriver}, delay::{FreeRtos, BLOCK}};
use esp_idf_sys::EspError;


const LCD_DRIVER_I2C_ADDR: u8 = 0x27;

enum LCDCommonCmd {
    LCD_CLEARDISPLAY = 0x01,
    LCD_ENTRYMODESET = 0x04,
    LCD_RETURNHOME = 0x02,
    LCD_DISPLAYCONTROL = 0x08,
    LCD_CURSORSHIFT = 0x10,
    LCD_FUNCTIONSET = 0x20,
    LCD_SETCGRAMLCD_DRIVER_I2C_LCD_ESS = 0x40,
    LCD_SETDDRAMLCD_DRIVER_I2C_LCD_ESS = 0x80
}

enum LCDDisplayEntryModeFlag {
    LCD_ENTRYRIGHT = 0x00,
    LCD_ENTRYLEFT = 0x02,
    
}

enum LCDDisplayEntryShiftFlag {
    LCD_ENTRYSHIFTINCREMENT = 0x01,
    LCD_ENTRYSHIFTDECREMENT = 0x00,
}

enum LCDDisplayOnOffControl {
    LCD_DISPLAYON = 0x04,
    LCD_DISPLAYOFF = 0x00,
}

enum LCDCursonOnOffControl {
    LCD_CURSORON = 0x02,
    LCD_CURSOROFF = 0x00,
}

enum LCDCursorBlinkControl {
    LCD_BLINKON = 0x01,
    LCD_BLINKOFF = 0x00,
}


enum LCDMoveMode {
    // flags for display/cursor shift
    LCD_DISPLAYMOVE = 0x08,
    LCD_CURSORMOVE = 0x00,
}

enum LCDMoveControl {
    LCD_MOVERIGHT = 0x04,
    LCD_MOVELEFT = 0x00,
}

enum LCDBacklitControl {
    // flags for backlight control
    LCD_BACKLIGHT = 0x08,
    LCD_NOBACKLIGHT = 0x00,
}

enum LCDPinMessageLength {
    // flags for function set
    LCD_8BITMODE = 0x10,
    LCD_4BITMODE = 0x00,
}

enum LCDLineControl {
    LCD_2LINE = 0x08,
    LCD_1LINE = 0x00,
}

enum LCDDotCounts {
    LCD_5x10DOTS = 0x04,
    LCD_5x8DOTS = 0x00,
}

pub struct LCD {
    content: String,
}

impl LCD {
    pub fn new() -> LCD {
        return LCD { content: String::from("") };
    }

    pub fn init_lcd(&mut self, i2c: &mut I2cDriver) {
        println!("Init LCD");

        FreeRtos::delay_ms(50);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x30], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(5);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x30], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(1);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x30], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(10);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x20], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(10);

        FreeRtos::delay_ms(50);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x28], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(5);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x08], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(1);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x01], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(10);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x06], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(10);
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x0c], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
        FreeRtos::delay_ms(10); 


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

        return i2c.write(LCD_DRIVER_I2C_ADDR, &data, BLOCK);

    }

    pub fn send_data(&mut self, i2c: &mut I2cDriver, data_unit: u8) -> Result<(), EspError> {
        let mut data: [u8; 4] = [0, 0, 0, 0];

        let data_u: u8 = data_unit & 0xf0;
        let data_l: u8 = (data_unit << 4) & 0xf0;

        data[0] = data_u|0x0D;  //en=1, rs=1
        data[1] = data_u|0x09;  //en=0, rs=1
        data[2] = data_l|0x0D;  //en=1, rs=1
        data[3] = data_l|0x09;  //en=0, rs=1

        return i2c.write(LCD_DRIVER_I2C_ADDR, &data, BLOCK);
    }

    pub fn send_string(&mut self,  i2c: &mut I2cDriver, str: String) {
        for chr in str.chars() {
            self.send_data(i2c, chr as u8).unwrap();
        }
    }

    pub fn turn_off_lcd_backlit(&mut self,  i2c: &mut I2cDriver){
        i2c.write(LCD_DRIVER_I2C_ADDR, &[0, 0x08], BLOCK).unwrap_or_else(|_| println!("LCD init fails"));
    }

    // pub fn lcd_send_cmd(&mut self, cmd: u8) {
    //     self.i2c.write(LCD_DRIVER_I2C_LCD_DRIVER_I2C_LCD_ESSESS, )
    // }

}