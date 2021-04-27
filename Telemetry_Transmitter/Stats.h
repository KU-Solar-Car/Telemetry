#ifndef STATS_H
#define STATS_H

struct TelemetryData
{
  enum Key: int
  {
    BATT_VOLTAGE,
    BATT_CURRENT,
    BATT_TEMP,
    BMS_FAULT,
    GPS_TIME,
    GPS_LAT,
    GPS_LON,
    GPS_VEL_EAST,
    GPS_VEL_NOR,
    GPS_VEL_UP,
    GPS_SPD,
    SOLAR_VOLTAGE,
    SOLAR_CURRENT,
    MOTOR_SPD,
    _LAST
    // TODO: Add motor temperature
  };

  struct
  {
    union {
      double doubleVal;
      unsigned uIntVal;
      bool boolVal;
    };
    bool present;
  } arr[Key::_LAST];

  void setDouble(int key, double value);
  void setUInt(int key, unsigned value);
  void setBool(int key, bool value);
  void unset(int key);


  double getDouble(int key);
  unsigned getUInt(int key);
  bool getBool(int key);
  
  bool isPresent(int key);

  void clear();
  
};


#endif
