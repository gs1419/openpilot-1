#pragma once

#include <memory>
#include <string>
#include <optional>

#include <QObject>
#include <QTimer>
#include <QColor>
#include <QFuture>
#include <QPolygonF>
#include <QTransform>

#include "cereal/messaging/messaging.h"
#include "common/modeldata.h"
#include "common/params.h"
#include "common/timing.h"

const int bdr_s = 15;
const int header_h = 420;
const int footer_h = 280;

const int UI_FREQ = 20; // Hz
typedef cereal::CarControl::HUDControl::AudibleAlert AudibleAlert;

const mat3 DEFAULT_CALIBRATION = {{ 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0 }};

const vec3 default_face_kpts_3d[] = {
  {-5.98, -51.20, 8.00}, {-17.64, -49.14, 8.00}, {-23.81, -46.40, 8.00}, {-29.98, -40.91, 8.00}, {-32.04, -37.49, 8.00},
  {-34.10, -32.00, 8.00}, {-36.16, -21.03, 8.00}, {-36.16, 6.40, 8.00}, {-35.47, 10.51, 8.00}, {-32.73, 19.43, 8.00},
  {-29.30, 26.29, 8.00}, {-24.50, 33.83, 8.00}, {-19.01, 41.37, 8.00}, {-14.21, 46.17, 8.00}, {-12.16, 47.54, 8.00},
  {-4.61, 49.60, 8.00}, {4.99, 49.60, 8.00}, {12.53, 47.54, 8.00}, {14.59, 46.17, 8.00}, {19.39, 41.37, 8.00},
  {24.87, 33.83, 8.00}, {29.67, 26.29, 8.00}, {33.10, 19.43, 8.00}, {35.84, 10.51, 8.00}, {36.53, 6.40, 8.00},
  {36.53, -21.03, 8.00}, {34.47, -32.00, 8.00}, {32.42, -37.49, 8.00}, {30.36, -40.91, 8.00}, {24.19, -46.40, 8.00},
  {18.02, -49.14, 8.00}, {6.36, -51.20, 8.00}, {-5.98, -51.20, 8.00},
};

const Rect rec_btn = {1745, 905, 140, 140};
const Rect laneless_btn = {1585, 905, 140, 140};
const Rect monitoring_btn = {50, 770, 140, 150};
const Rect stockui_btn = {15, 15, 184, 202};
const Rect tuneui_btn = {1720, 15, 184, 202};
const Rect livetunepanel_left_above_btn = {470, 570, 210, 170};
const Rect livetunepanel_right_above_btn = {1240, 570, 210, 170};
const Rect livetunepanel_left_btn = {470, 745, 210, 170};
const Rect livetunepanel_right_btn = {1240, 745, 210, 170};
const Rect speedlimit_btn = {220, 15, 190, 190};

struct Alert {
  QString text1;
  QString text2;
  QString type;
  cereal::ControlsState::AlertSize size;
  AudibleAlert sound;

  bool equal(const Alert &a2) {
    return text1 == a2.text1 && text2 == a2.text2 && type == a2.type && sound == a2.sound;
  }

  static Alert get(const SubMaster &sm, uint64_t started_frame) {
    const cereal::ControlsState::Reader &cs = sm["controlsState"].getControlsState();
    if (sm.updated("controlsState")) {
      return {cs.getAlertText1().cStr(), cs.getAlertText2().cStr(),
              cs.getAlertType().cStr(), cs.getAlertSize(),
              cs.getAlertSound()};
    } else if ((sm.frame - started_frame) > 5 * UI_FREQ) {
      const int CONTROLS_TIMEOUT = 5;
      const int controls_missing = (nanos_since_boot() - sm.rcv_time("controlsState")) / 1e9;

      // Handle controls timeout
      if (sm.rcv_frame("controlsState") < started_frame) {
        // car is started, but controlsState hasn't been seen at all
        return {"openpilot Unavailable", "Waiting for controls to start",
                "controlsWaiting", cereal::ControlsState::AlertSize::MID,
                AudibleAlert::NONE};
      } else if (controls_missing > CONTROLS_TIMEOUT && !Hardware::PC()) {
        // car is started, but controls is lagging or died
        if (cs.getEnabled() && (controls_missing - CONTROLS_TIMEOUT) < 10) {
          return {"TAKE CONTROL IMMEDIATELY", "Controls Unresponsive",
                  "controlsUnresponsive", cereal::ControlsState::AlertSize::FULL,
                  AudibleAlert::WARNING_IMMEDIATE};
        } else {
          return {"Controls Unresponsive", "Reboot Device",
                  "controlsUnresponsivePermanent", cereal::ControlsState::AlertSize::MID,
                  AudibleAlert::NONE};
        }
      }
    }
    return {};
  }
};

typedef enum UIStatus {
  STATUS_DISENGAGED,
  STATUS_OVERRIDE,
  STATUS_ENGAGED,
  STATUS_WARNING,
  STATUS_ALERT,
} UIStatus;

const QColor bg_colors [] = {
  [STATUS_DISENGAGED] = QColor(0x17, 0x33, 0x49, 0xc8),
  [STATUS_OVERRIDE] = QColor(0x91, 0x9b, 0x95, 0x96),
  [STATUS_ENGAGED] = QColor(0x17, 0x86, 0x44, 0x96),
  [STATUS_WARNING] = QColor(0xDA, 0x6F, 0x25, 0x96),
  [STATUS_ALERT] = QColor(0xC9, 0x22, 0x31, 0x96),
  [STATUS_DND] = QColor(0x32, 0x32, 0x32, 0x96),
};

typedef struct UIScene {
  bool calibration_valid = false;
  bool calibration_wide_valid  = false;
  bool wide_cam = true;
  mat3 view_from_calib = DEFAULT_CALIBRATION;
  mat3 view_from_wide_calib = DEFAULT_CALIBRATION;
  cereal::PandaState::PandaType pandaType;

  std::string alertTextMsg1;
  std::string alertTextMsg2;
  std::string alertTextMsg3;
  float alert_blinking_rate;

  bool brakePress;
  bool gasPress;
  bool autoHold;
  bool touched = false;

  float gpsAccuracyUblox;
  float altitudeUblox;
  float bearingUblox;

  int cpuPerc;
  float cpuTemp;
  float ambientTemp;
  bool rightblindspot;
  bool leftblindspot;
  bool leftBlinker;
  bool rightBlinker;
  int blinker_blinkingrate;
  int tpms_blinkingrate = 120;
  int blindspot_blinkingrate = 120;
  int car_valid_status_changed = 0;
  float angleSteers;
  float desired_angle_steers;
  bool gap_by_speed_on;
  float steerRatio;
  bool brakeLights;
  bool steerOverride;
  float output_scale;
  int fanSpeed;
  int tpmsUnit;
  float tpmsPressureFl;
  float tpmsPressureFr;
  float tpmsPressureRl;
  float tpmsPressureRr;
  int lateralControlMethod;
  float radarDistance;
  bool standStill;
  int limitSpeedCamera = 0;
  float limitSpeedCameraDist = 0;
  int mapSign;
  int mapSignCam;
  float vSetDis;
  bool cruiseAccStatus;
  bool driverAcc;
  int laneless_mode;
  int recording_count;
  int recording_quality;
  bool monitoring_mode;
  bool forceGearD;
  bool opkr_livetune_ui;
  bool driving_record;
  float steer_actuator_delay;
  int cruise_gap;
  int dynamic_tr_mode;
  float dynamic_tr_value;
  bool touched2 = false;
  int brightness_off;
  int cameraOffset, pathOffset;
  int pidKp, pidKi, pidKd, pidKf;
  int indiInnerLoopGain, indiOuterLoopGain, indiTimeConstant, indiActuatorEffectiveness;
  int lqrScale, lqrKi, lqrDcGain;
  int torqueKp, torqueKf, torqueKi, torqueFriction, torqueMaxLatAccel;
  bool live_tune_panel_enable;
  int bottom_text_view;
  int live_tune_panel_list = 0;
  int list_count = 2;
  int nTime, autoScreenOff, brightness, awake;
  int nVolumeBoost = 0;
  bool read_params_once = false;
  bool nDebugUi1;
  bool nDebugUi2;
  bool nDebugUi3;
  bool nOpkrBlindSpotDetect;
  bool auto_gitpull = false;
  bool is_speed_over_limit = false;
  bool controlAllowed;
  bool steer_warning;
  bool stand_still;
  bool show_error;
  int display_maxspeed_time = 0;
  int navi_select;
  bool tmux_error_check = false;
  bool speedlimit_signtype;
  bool sl_decel_off;
  bool pause_spdlimit;
  float a_req_value;
  bool osm_enabled;
  int radar_long_helper;
  float engine_rpm;
  bool cal_view = false;
  float ctrl_speed;
  float accel;
  bool animated_rpm;
  int max_animated_rpm;
  bool stop_line;
  int gear_step;
  float charge_meter;
  float multi_lat_selected;
  int do_not_disturb_mode;
  bool depart_chime_at_resume;
  int comma_stock_ui;
  bool OPKR_Debug;
  // gps
  int satelliteCount;
  float gpsAccuracy;

  cereal::DeviceState::Reader deviceState;
  cereal::CarState::Reader car_state;
  cereal::ControlsState::Reader controls_state;
  cereal::CarState::GearShifter getGearShifter;
  cereal::LateralPlan::Reader lateral_plan;
  cereal::LiveENaviData::Reader live_enavi_data;
  cereal::LiveMapData::Reader live_map_data;
  cereal::LongitudinalPlan::Reader longitudinal_plan;


  // atom
  struct _LiveParams
  {
    float angleOffset;
    float angleOffsetAverage;
    float stiffnessFactor;
    float steerRatio;
  } liveParams;

  struct _LateralPlan
  {
    float laneWidth;
    int standstillElapsedTime = 0;

    float dProb;
    float lProb;
    float rProb;

    float angleOffset;
    bool lanelessModeStatus;
    float totalCameraOffset;
  } lateralPlan;

  struct _LiveENaviData
  {
    int eopkrspeedlimit;
    float eopkrsafetydist;
    int eopkrsafetysign;
    int eopkrturninfo;
    float eopkrdisttoturn;
    bool eopkrconalive;
    int eopkrroadlimitspeed;
    int eopkrlinklength;
    int eopkrcurrentlinkangle;
    int eopkrnextlinkangle;
    std::string eopkrroadname;
    bool eopkrishighway;
    bool eopkristunnel;
    std::string eopkr0;
    std::string eopkr1;
    std::string eopkr2;
    std::string eopkr3;
    std::string eopkr4;
    std::string eopkr5;
    std::string eopkr6;
    std::string eopkr7;
    std::string eopkr8;
    std::string eopkr9;
    int ewazealertid;
    int ewazealertdistance;
    int ewazeroadspeedlimit;
    int ewazecurrentspeed;
    std::string ewazeroadname;
    int ewazenavsign;
    int ewazenavdistance;
    std::string ewazealerttype;
  } liveENaviData;

  struct _LiveMapData
  {
    float ospeedLimit;
    float ospeedLimitAhead;
    float ospeedLimitAheadDistance;
    float oturnSpeedLimit;
    float oturnSpeedLimitEndDistance;
    int oturnSpeedLimitSign;
    std::string ocurrentRoadName;
    std::string oref;
    //float turnSpeedLimitsAhead[16]; // List
    //float turnSpeedLimitsAheadDistances[16]; // List
    //int turnSpeedLimitsAheadSigns[16]; // List
  } liveMapData;

  struct _LongitudinalPlan
  {
    float e2ex[13] = {0};
    float lead0[13] = {0};
    float lead1[13] = {0};
    float cruisetg[13] = {0};
    float stopline[13] = {0};
    float stopprob;
  } longitudinalPlan;


  // modelV2
  float lane_line_probs[4];
  float road_edge_stds[2];
  QPolygonF track_vertices;
  QPolygonF lane_line_vertices[4];
  QPolygonF road_edge_vertices[2];

  float lane_blindspot_probs[2];
  line_vertices_data lane_blindspot_vertices[2];
  line_vertices_data stop_line_vertices;

  // lead
  QPointF lead_vertices[2];

  // DMoji state
  float driver_pose_vals[3];
  float driver_pose_diff[3];
  float driver_pose_sins[3];
  float driver_pose_coss[3];
  vec3 face_kpts_draw[std::size(default_face_kpts_3d)];

  float light_sensor;
  bool started, ignition, is_metric, map_on_left, longitudinal_control;
  uint64_t started_frame;
} UIScene;

class UIState : public QObject {
  Q_OBJECT

public:
  UIState(QObject* parent = 0);
  void updateStatus();
  inline bool worldObjectsVisible() const {
    return sm->rcv_frame("liveCalibration") > scene.started_frame;
  }
  inline bool engaged() const {
    return scene.started && (*sm)["controlsState"].getControlsState().getEnabled();
  }

  void setPrimeType(int type);
  inline int primeType() const { return prime_type; }

  int fb_w = 0, fb_h = 0;

  std::unique_ptr<SubMaster> sm;

  UIStatus status;
  UIScene scene = {};

  bool awake;
  QString language;

  bool is_OpenpilotViewEnabled = false;

  QTransform car_space_transform;

signals:
  void uiUpdate(const UIState &s);
  void offroadTransition(bool offroad);
  void primeTypeChanged(int prime_type);

private slots:
  void update();

private:
  QTimer *timer;
  bool started_prev = false;
  int prime_type = -1;
};

UIState *uiState();

// device management class
class Device : public QObject {
  Q_OBJECT

public:
  Device(QObject *parent = 0);

private:
  bool awake = false;
  int interactive_timeout = 0;
  bool ignition_on = false;
  int last_brightness = 0;
  FirstOrderFilter brightness_filter;
  QFuture<void> brightness_future;

  int sleep_time = -1;

  void updateBrightness(const UIState &s);
  void updateWakefulness(const UIState &s);
  bool motionTriggered(const UIState &s);
  void setAwake(bool on);

signals:
  void displayPowerChanged(bool on);
  void interactiveTimout();

public slots:
  void resetInteractiveTimout();
  void update(const UIState &s);
};

void ui_update_params(UIState *s);
int get_path_length_idx(const cereal::XYZTData::Reader &line, const float path_height);
void update_model(UIState *s,
                  const cereal::ModelDataV2::Reader &model,
                  const cereal::UiPlan::Reader &plan);
void update_dmonitoring(UIState *s, const cereal::DriverStateV2::Reader &driverstate, float dm_fade_state, bool is_rhd);
void update_leads(UIState *s, const cereal::RadarState::Reader &radar_state, const cereal::XYZTData::Reader &line);
void update_line_data(const UIState *s, const cereal::XYZTData::Reader &line,
                      float y_off, float z_off, QPolygonF *pvd, int max_idx, bool allow_invert);
