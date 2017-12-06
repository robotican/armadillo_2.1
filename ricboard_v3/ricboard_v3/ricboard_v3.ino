#include "ric_settings.h"
#include "ric_communicator.h"
#include "protocol.h"
#include "strober.h"
#include "timer.h"
#include "ultrasonic.h"
#include "imu.h"
#include "laser.h"
#include "gps.h"

/* this cpp acts as board manager */

/* DO NOT use Serial.print, because */
/* communicator use it for communication with pc */

Timer send_keepalive_timer, 
      get_keepalive_timer,
      send_readings_timer;
bool got_keepalive;
Strober strober;

/* front ultrasonic */
Ultrasonic ultrasonic;
Imu imu;
Laser laser;
Gps gps;

/******************************************************/

void setup() 
{
  
  communicator::init(BAUDRATE);
  send_keepalive_timer.start(SEND_KA_INTERVAL);
  get_keepalive_timer.start(GET_KA_INTERVAL);
  send_readings_timer.start(SEND_READINGS_INTERVAL);
  
  ultrasonic.init(ULTRASONIC_PIN);
  if (!imu.init())
    Serial.println("imu failed");//log("imu failed", protocol::logger::Code::ERROR);

  delay(5);

  laser.init();

  delay(5);

  gps.init();

  delay(5);

  /* pin mode must be invoked after i2c devices (arduino bug) */
  pinMode(INDICATOR_LED, OUTPUT);
  strober.setNotes(Strober::Notes::BLINK_SLOW);
}

/******************************************************/

void loop() 
{
  strober.play(INDICATOR_LED);
  keepAliveAndRead();  
  sendReadings();
}

/******************************************************/

void sendReadings()
{
  
  /* read IMU if available. IMU read must be called */
  /* as fast as possible (no delays*                */
  protocol::imu imu_pkg;
  bool valid_imu = false;
  if (imu.read(imu_pkg)) //if imu ready, send it
    valid_imu = true;
  
     
  if (send_readings_timer.finished())
  {
    /* ULTRASONIC */
    protocol::header ultrasonic_header;
    ultrasonic_header.type = protocol::Type:: ULTRASONIC;
    protocol::ultrasonic ultrasonic_pkg;
    ultrasonic_pkg.distance_mm = ultrasonic.readDistanceMm();
    communicator::ric::sendPkg(ultrasonic_header, ultrasonic_pkg, sizeof(ultrasonic_pkg));

    /* IMU */
    if (valid_imu)
    {
      protocol::header imu_header;
      imu_header.type = protocol::Type:: IMU;
      communicator::ric::sendPkg(imu_header, imu_pkg, sizeof(imu_pkg));
      
      //Serial.print("roll: "); Serial.println(imu_pkg.roll * 180 / M_PI);
      //Serial.print("pitch: "); Serial.println(imu_pkg.pitch * 180 / M_PI);
      //Serial.print("yaw: "); Serial.println(imu_pkg.yaw * 180 / M_PI);
    }

    /* LASER */
    uint16_t laser_read = laser.read();
    if (laser_read != (uint16_t)Laser::Code::ERROR)
    {
      protocol::header laser_header;
      laser_header.type = protocol::Type:: LASER;
      protocol::laser laser_pkg;
      laser_pkg.distance_mm = laser_read;
      communicator::ric::sendPkg(laser_header, laser_pkg, sizeof(laser_pkg));
    }

    /* GPS */
    protocol::gps gps_pkg;
    bool valid_gps = gps.read(gps_pkg);
    if (valid_gps)
    {
      protocol::header gps_header;
      gps_header.type = protocol::Type::GPS;
      communicator::ric::sendPkg(gps_header, gps_pkg, sizeof(gps_pkg));
    }

    
    send_readings_timer.startOver();
  }
}

/******************************************************/

void keepAliveAndRead()
{
  if (send_keepalive_timer.finished())
  {
    /* send keep alive */
    protocol::header ka_header;
    ka_header.type = protocol::Type:: KEEP_ALIVE;
    protocol::keepalive ka_pkg;
    communicator::ric::sendPkg(ka_header, ka_pkg, sizeof(ka_pkg));
    
    send_keepalive_timer.startOver();
  }

  if (get_keepalive_timer.finished())
  {
    if (got_keepalive) //connected to pc
    {
      strober.setNotes(Strober::Notes::STROBE);
      got_keepalive = false;
    }
    else //disconnected from pc
      strober.setNotes(Strober::Notes::BLINK_SLOW);
    get_keepalive_timer.startOver();
  }

  protocol::header incoming_header;
  if (communicator::ric::readPkg(incoming_header, sizeof(incoming_header)))
  {
    handleHeader(incoming_header);
  }
}

/******************************************************/

void handleHeader(const protocol::header &h)
{
    switch (h.type)
    {
        case protocol::Type::KEEP_ALIVE:
            got_keepalive = true;
            break;
        case protocol::Type::SERVO:
            protocol::servo servo_pkg;
            communicator::ric::readPkg(servo_pkg, sizeof(servo_pkg));
            log("got servo", (protocol::logger::Code)servo_pkg.cmd);
            //TODO: SEND SERVO CMD PKG TO SERVO CLASS --------------------------
            break;
    }
}

/******************************************************/
void log(const char* msg_str, protocol::logger::Code code)
{
    protocol::header logger_header;
    logger_header.type = protocol::Type::LOGGER;
    protocol::logger logger_pkg;
    strcpy(logger_pkg.msg, msg_str);
    
    logger_pkg.code = code;
    communicator::ric::sendPkg(logger_header, logger_pkg, sizeof(logger_pkg));
}
