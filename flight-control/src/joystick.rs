



pub enum Direction {
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