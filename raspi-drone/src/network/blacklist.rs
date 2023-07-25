use std::collections::LinkedList;
use std::ops::Add;

pub struct BlackList<AddressSize> {
    blacklist: Vec<AddressSize>
}


impl <AddressSize> BlackList<AddressSize>
    where AddressSize: PartialEq  + Add
{

    pub fn new() -> Self {
        BlackList {
            blacklist: Vec::new()
        }
    }

    pub fn add_to_blacklist(&mut self, address: AddressSize) {
        self.blacklist.push_back(address);
    }

    pub fn remove_from_blacklist(&mut self, address: AddressSize) {
        self.blacklist.remove(address);
    }

    pub fn is_blacklisted(&self, address: AddressSize) -> bool {
        self.blacklist.contains(&address)
    }
}