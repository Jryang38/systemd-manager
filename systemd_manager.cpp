#include "systemd_manager.h"
#include <iostream>


/*  timeout for the reload. */
#define DAEMON_RELOAD_TIMEOUT_SEC (180 * 1000000ULL)

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
                             &reply,                                            /* response message */
                             unit_actions[actionType].msgFormat,                /* input message format */
                             unit_name.c_str(),                                 /* target unit */
                             unit_actions[actionType].mode);                    /* action mode */

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
                             SYSTEMD_SERVICE_NAME,                               /* service to contact */
                             SYSTEMD_OBJECT_PATH_NAME,                           /* object path */
                             SYSTEMD_MANAGER_INTERFACE_NAME,                     /* interface name */
                             SYSTEMD_LOAD_UNIT_METHOD,                           /* method name */
                             &error,                                             /* object to return error in */
                             &reply,                                             /* response message */
                             "s",                                                /* input message format */
                             unit_name.c_str()                                   /* target unit */
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
                                     SYSTEMD_SERVICE_NAME,                       /* service to contact */
                                     unit_path,                                  /* object path */
                                     SYSTEMD_UNIT_INTERFACE_NAME,                /* interface name */
                                     "ActiveState",                              /* method name */
                                     &error,                                     /* object to return error in */
                                     &active_state                               /* activate state */ 
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
                             nullptr,                                           /* response */
                             nullptr                                            /* target unit */
                            );

    if (ret < 0)
    {
        std::cerr << "Failed to issue method call: " << error.message << std::endl;
        goto finish;
    }
    std::cout << "Reboot System..." << std::endl;

finish:
    sd_bus_error_free(&error);

    return ret;
}

int SystemdManager::daemon_reload()
{
    int ret = -1;
    sd_bus_message *m = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus *pBus = NULL;

    sd_bus_open_system(&pBus);

    ret = sd_bus_message_new_method_call(pBus,
                                         &m,
                                         SYSTEMD_SERVICE_NAME,                              /* service to contact */
                                         SYSTEMD_OBJECT_PATH_NAME,                          /* object path */
                                         SYSTEMD_MANAGER_INTERFACE_NAME,                    /* interface name */
                                         SYSTEMD_DAEMON_RELOAD_METHOD                       /* method name */
                                        );

    if (ret < 0)
    {
        std::cerr << "Failed to issue deamon-reload call" << std::endl;
        goto finish;
    }

    /* Reloading the daemon may take long, hence set a longer timeout here */
    ret = sd_bus_call(pBus, m, DAEMON_RELOAD_TIMEOUT_SEC, &error, NULL);

    if (ret < 0)
    {
        std::cerr << "Failed to run systemd deamon-reload: " << error.message << std::endl;
        goto finish;
    }
    std::cout << "Reload System manager Configuration..." << std::endl;

finish:
    sd_bus_error_free(&error);
    sd_bus_unref(pBus);

    return ret;
}

int main(int argc, char *argv[])
{
    SystemdManager s;

    if (argc <= 1) {
        std::cout << "Usage: systemd-manager <service name>" << std::endl;
        return 1;
    }
    s.daemon_reload();

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
