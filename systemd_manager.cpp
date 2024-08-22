#include "systemd_manager.h"
#include <iostream>

SystemdManager::SystemdManager()
{
    sd_bus_open_system(&m_bus);
}

SystemdManager::~SystemdManager()
{
    sd_bus_unref(m_bus);
}

int SystemdManager::toggleService(std::string unit_name, UnitActionType actionType)
{
    int ret = -1;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    char *object = nullptr;

    if (!m_bus)
    {
        goto finish;
    }

    ret = sd_bus_call_method(m_bus,
                             SYSTEMD_SERVICE_NAME,                              /* service to contact */
                             SYSTEMD_OBJECT_PATH_NAME,                          /* object path */
                             SYSTEMD_MANAGER_INTERFACE_NAME,                    /* interface name */
                             unit_actions[actionType].method,                   /* method name */
                             &error,                                            /* object to return error in */
                             &reply,
                             unit_actions[actionType].arg,
                             unit_name.c_str(),
                             unit_actions[actionType].mode);

    if (ret < 0)
    {
        std::cerr << "Failed to issue method call: " << error.message << std::endl;
        goto finish;
    }

    if ((actionType != SYSTEMD_FREEZE_UNIT) && (actionType != SYSTEMD_THAW_UNIT))
    {
        /* Parse the response message */
        ret = sd_bus_message_read(reply, "o", &object);
        if (ret < 0)
        {
            std::cerr << "Failed to parse response message: " << std::strerror(-ret) << std::endl;
            goto finish;
        }
        std::cout << "Object " << object << std::endl;
    }

    std::cout << unit_actions[actionType].verb << unit_name.c_str() << " succeed!" << std::endl;

finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(reply);

    return ret;
}

bool SystemdManager::isActive(std::string unit_name)
{
    int ret;
    bool state = false;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    char *unit_path = nullptr;
    char *active_state = nullptr;

    if (!m_bus)
    {
        goto finish;
    }

    ret = sd_bus_call_method(m_bus,
                             SYSTEMD_SERVICE_NAME, /* service to contact */
                             SYSTEMD_OBJECT_PATH_NAME, /* object path */
                             SYSTEMD_MANAGER_INTERFACE_NAME, /* interface name */
                             SYSTEMD_LOAD_UNIT_METHOD, /* method name */
                             &error,/* object to return error in */
                             &reply,
                             "s",
                             unit_name.c_str()
                             );

    if (ret < 0) {
        std::cerr << "Failed to issue method call: " << error.message << std::endl;
        goto finish;
    }

    /* Parse the response message */
    ret = sd_bus_message_read(reply, "o", &unit_path);
    if (ret < 0) {
        std::cerr <<  "Failed to parse response message: " << std::strerror(-ret) << std::endl;
        goto finish;
    }

    std::cout << "Unit path :" << unit_path << std::endl;

    ret = sd_bus_get_property_string(m_bus,
                             SYSTEMD_SERVICE_NAME, /* service to contact */
                             unit_path, /* object path */
                             SYSTEMD_UNIT_INTERFACE_NAME, /* interface name */
                             "ActiveState",
                             &error,/* object to return error in */
                             &active_state
                             );

    if (ret < 0) {
        std::cerr << "Failed to issue method call: " << error.message << std::endl;
        goto finish;
    }

    if (strcmp(active_state, "active") == 0)
    {
        state = true;
    }

    if (state)
        std::cout<< unit_name.c_str() << "is alive!" << std::endl;
    else
	    std::cout<< unit_name.c_str() << "is not alive!" << std::endl;
finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(reply);
    if (active_state) {
        free(active_state);
    }

    return state;
}

int SystemdManager::reboot()
{
    int ret = -1;
    sd_bus_error error = SD_BUS_ERROR_NULL;

    if (!m_bus)
    {
        goto finish;
    }

    ret = sd_bus_call_method(m_bus,
                             SYSTEMD_SERVICE_NAME,                              /* service to contact */
                             SYSTEMD_OBJECT_PATH_NAME,                          /* object path */
                             SYSTEMD_MANAGER_INTERFACE_NAME,                    /* interface name */
                             SYSTEMD_REBOOT_METHOD,                             /* method name */
                             &error,                                            /* object to return error in */
                             nullptr, nullptr);

    if (ret < 0)
    {
        std::cerr << "Failed to issue method call: " << error.message << std::endl;
        goto finish;
    }

finish:
    sd_bus_error_free(&error);
    return ret;
}

int main(int argc, char *argv[])
{
    SystemdManager s;

    if (argc <= 1) {
        printf("Usage: systemd-manager <service name>\n");
        return 1;
    }

    if ( s.isActive(argv[1]) )
    {
        s.toggleService(argv[1], SYSTEMD_STOP_UNIT);
    }
    else
    {
        s.toggleService(argv[1], SYSTEMD_START_UNIT);
    }

    s.toggleService(argv[1], SYSTEMD_RESTART_UNIT);

    s.toggleService(argv[1], SYSTEMD_FREEZE_UNIT);

    s.toggleService(argv[1], SYSTEMD_THAW_UNIT);

	return 0;
}
