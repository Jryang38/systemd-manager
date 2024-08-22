#ifndef SYSTEMD_MANAGER_H
#define SYSTEMD_MANAGER_H

#include <systemd/sd-daemon.h>
#include <systemd/sd-bus.h>
#include <stdbool.h>
#include <cstring>
#include <string>

#define SYSTEMD_SERVICE_NAME              "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH_NAME          "/org/freedesktop/systemd1"

#define SYSTEMD_UNIT_INTERFACE_NAME       "org.freedesktop.systemd1.Unit"
#define SYSTEMD_SERVICE_INTERFACE_NAME    "org.freedesktop.systemd1.Service"
#define SYSTEMD_MANAGER_INTERFACE_NAME    "org.freedesktop.systemd1.Manager"

#define SYSTEMD_REBOOT_METHOD             "Reboot"

#define SYSTEMD_LOAD_UNIT_METHOD          "LoadUnit"

typedef enum {
    SYSTEMD_START_UNIT = 0,
    SYSTEMD_STOP_UNIT = 1,
    SYSTEMD_RESTART_UNIT = 2,
    SYSTEMD_FREEZE_UNIT = 3,
    SYSTEMD_THAW_UNIT = 4
} UnitActionType;

static const struct {
    const char *verb;      /* systemctl verb */
    const char *method;    /* Name of the specific D-Bus method */
    const char *arg;    /* Name of the specific D-Bus method */
    const char *mode;    /* Name of the specific D-Bus method */
} unit_actions[] = {
    { "start",                 "StartUnit",              "ss",                 "replace"},
    { "stop",                  "StopUnit",               "ss",                 "replace"},
    { "restart",               "RestartUnit",            "ss",                 "replace"},
    { "freeze",                "FreezeUnit",             "s",                  NULL },
    { "thaw",                  "ThawUnit",               "s",                  NULL },
};

class SystemdManager
{
public:
    SystemdManager();
    ~SystemdManager();
    int toggleService(std::string unit_name, UnitActionType actionType);
    bool isActive(std::string unit_name);
    int reboot();

private:
    sd_bus *m_bus = nullptr;
};

#endif // SYSTEMD_MANAGER_H
