pub enum VehicleType {
    BaseStation,
    Quadcopter,
    FlyingWing,
    FixedWing,
    Rover,
}

/// Containing all the important information about the device.
pub struct DeviceSelf<AddressSize> {
    pub device_type: VehicleType,
    pub address: Option<AddressSize>,
}
