/*
 * Wazuh Module Configuration
 * Copyright (C) 2015-2019, Wazuh Inc.
 * April 25, 2016.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "wazuh_modules/wmodules.h"

static const char *XML_NAME = "name";

// Read wodle element

int Read_WModule(const OS_XML *xml, xml_node *node, void *d1, void *d2, int cfg_type, char **output)
{
    wmodule **wmodules = (wmodule**)d1;
    int agent_cfg = d2 ? *(int *)d2 : 0;
    wmodule *cur_wmodule;
    xml_node **children = NULL;
    wmodule *cur_wmodule_exists;
    char message[OS_FLSIZE];

    if (!node->attributes[0]) {
        if (output == NULL) {
            merror("No such attribute '%s' at module.", XML_NAME);
        } else {
            snprintf(message, OS_FLSIZE + 1,
                "No such attribute '%s' at module.",
                XML_NAME);
            wm_strcat(output, message, '\n');
        }
        return OS_INVALID;
    }

    if (strcmp(node->attributes[0], XML_NAME)) {
        if (output == NULL) {
            merror("Module attribute is not '%s'.", XML_NAME);
        } else {
            snprintf(message, OS_FLSIZE + 1,
                "Module attribute is not '%s'.",
                XML_NAME);
            wm_strcat(output, message, '\n');
        }
        return OS_INVALID;
    }

    // Allocate memory

    if ((cur_wmodule = *wmodules)) {
        cur_wmodule_exists = *wmodules;
        int found = 0;

        while (cur_wmodule_exists) {
            if(cur_wmodule_exists->tag) {
                if (!output) {
                    if(strcmp(cur_wmodule_exists->tag,node->values[0]) == 0) {
                        cur_wmodule = cur_wmodule_exists;
                        found = 1;
                        break;
                    }
                }
            }
            cur_wmodule_exists = cur_wmodule_exists->next;
        }

        if(!found) {
            while (cur_wmodule->next)
                cur_wmodule = cur_wmodule->next;

            os_calloc(1, sizeof(wmodule), cur_wmodule->next);
            cur_wmodule = cur_wmodule->next;
        }
    } else
        *wmodules = cur_wmodule = calloc(1, sizeof(wmodule));

    if (!cur_wmodule) {
        if (output == NULL) {
            merror(MEM_ERROR, errno, strerror(errno));
        } else {
            snprintf(message, OS_FLSIZE + 1,
                "Could not acquire memory due to [(%d)-(%s)].",
                errno, strerror(errno));
            wm_strcat(output, message, '\n');
        }
        return (OS_INVALID);
    }

    // Get children

    if (children = OS_GetElementsbyNode(xml, node), !children) {
        if (output == NULL){
            mdebug1("Empty configuration for module '%s'.", node->values[0]);
        }
    }

    // Select module by name

    //osQuery monitor module
    if (!strcmp(node->values[0], WM_OSQUERYMONITOR_CONTEXT.name)) {
        if (wm_osquery_monitor_read(children, cur_wmodule, output) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }

    else if (!strcmp(node->values[0], WM_OSCAP_CONTEXT.name)) {
        if (wm_oscap_read(xml, children, cur_wmodule) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }
#ifdef ENABLE_SYSC
    else if (!strcmp(node->values[0], WM_SYS_CONTEXT.name)) {
        if (wm_sys_read(children, cur_wmodule) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }
#endif
    else if (!strcmp(node->values[0], WM_COMMAND_CONTEXT.name)) {
        if (wm_command_read(children, cur_wmodule, agent_cfg) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }
#ifdef ENABLE_CISCAT
    else if (!strcmp(node->values[0], WM_CISCAT_CONTEXT.name)) {
        if (wm_ciscat_read(xml, children, cur_wmodule, output) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }
#endif
    else if ( (!strcmp(node->values[0], WM_AWS_CONTEXT.name) || !strcmp(node->values[0], "aws-cloudtrail")) &&
        (cfg_type != CAGENT_CGFILE) && (cfg_type != CRMOTE_CONFIG) ) {
#ifndef WIN32
            if (!strcmp(node->values[0], "aws-cloudtrail")) {
                if (output == NULL) {
                    mwarn("Module name 'aws-cloudtrail' is deprecated. Change it to '%s'.", WM_AWS_CONTEXT.name);
                } else {
                    snprintf(message, OS_FLSIZE + 1,
                        "WARNING: Module name 'aws-cloudtrail' is deprecated. Change it to '%s'.",
                        WM_AWS_CONTEXT.name);
                    wm_strcat(output, message, '\n');
                }
            }
            if (wm_aws_read(xml, children, cur_wmodule) < 0) {
                OS_ClearNode(children);
                return OS_INVALID;
            }
#else
        if (output == NULL) {
            mwarn("The '%s' module is not available on Windows systems. Ignoring.", node->values[0]);
        } else {
            snprintf(message, OS_FLSIZE + 1,
                "WARNING: The '%s' module is not available on Windows systems. Ignoring.",
                node->values[0]);
            wm_strcat(output, message, '\n');
        }
#endif
    } else if ( (cfg_type != CRMOTE_CONFIG) && (!strcmp(node->values[0], "docker-listener")) ) {
#ifndef WIN32
            if (wm_docker_read(children, cur_wmodule) < 0) {
                OS_ClearNode(children);
                return OS_INVALID;
            }
#else
        if (output == NULL) {
            mwarn("The '%s' module is not available on Windows systems. Ignoring it.", node->values[0]);
        } else {
            snprintf(message, OS_FLSIZE + 1,
                "WARNING: The '%s' module is not available on Windows systems. Ignoring it",
                node->values[0]);
            wm_strcat(output, message, '\n');
        }
#endif
    }
#ifndef WIN32
#ifndef CLIENT
    else if (!strcmp(node->values[0], WM_VULNDETECTOR_CONTEXT.name) && (cfg_type != CAGENT_CGFILE) && (cfg_type != CRMOTE_CONFIG)) {
        if (wm_vuldet_read(xml, children, cur_wmodule) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    } else if (!strcmp(node->values[0], WM_AZURE_CONTEXT.name) && (cfg_type != CAGENT_CGFILE) && (cfg_type != CRMOTE_CONFIG)) {
        if (wm_azure_read(xml, children, cur_wmodule) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    } else if (!strcmp(node->values[0], WM_KEY_REQUEST_CONTEXT.name) && (cfg_type != CAGENT_CGFILE) && (cfg_type != CRMOTE_CONFIG)) {
        if (wm_key_request_read(children, cur_wmodule) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }
#endif
#endif
    else {
        if(!strcmp(node->values[0], VU_WM_NAME) || !strcmp(node->values[0], AZ_WM_NAME) ||
            !strcmp(node->values[0], KEY_WM_NAME)) {
            if (output == NULL) {
                mwarn("The '%s' module only works for the manager.", node->values[0]);
            } else {
                snprintf(message, OS_FLSIZE + 1,
                    "WARNING: The '%s' module only works for the manager.",
                    node->values[0]);
                wm_strcat(output, message, '\n');
            }
        } else if ( !strcmp(node->values[0], WM_AWS_CONTEXT.name) || !strcmp(node->values[0], "aws-cloudtrail") ) {
            if (output == NULL) {
                mwarn("The 'AWS' module only works for the manager.");
            } else {
                wm_strcat(output, "WARNING: The 'AWS' module only works for the manager.", '\n');
            }
        } else {
            char *type_str = NULL;
            type_str = cfg_type == CAGENT_CGFILE ? strdup("agent") : (cfg_type == CRMOTE_CONFIG ? strdup("remote") : strdup("manager"));
            if (output == NULL) {
                merror("Unknown module '%s' for the %s configuration file.", node->values[0], type_str);
            } else {
                snprintf(message, OS_FLSIZE + 1,
                    "Unknown module '%s' for the %s configuration file.",
                    node->values[0], type_str);
                wm_strcat(output, message, '\n');
            }
            os_free(type_str);
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }

    OS_ClearNode(children);
    return 0;
}

int Read_SCA(const OS_XML *xml, xml_node *node, void *d1, char **output)
{
    wmodule **wmodules = (wmodule**)d1;
    wmodule *cur_wmodule;
    xml_node **children = NULL;
    wmodule *cur_wmodule_exists;
    char message[OS_FLSIZE];

    // Allocate memory
    if ((cur_wmodule = *wmodules)) {
        cur_wmodule_exists = *wmodules;
        int found = 0;

        while (cur_wmodule_exists) {
            if(cur_wmodule_exists->tag) {
                if (!output) {
                    if(strcmp(cur_wmodule_exists->tag,node->element) == 0) {
                        cur_wmodule = cur_wmodule_exists;
                        found = 1;
                        break;
                    }
                }
            }
            cur_wmodule_exists = cur_wmodule_exists->next;
        }

        if(!found) {
            while (cur_wmodule->next)
                cur_wmodule = cur_wmodule->next;

            os_calloc(1, sizeof(wmodule), cur_wmodule->next);
            cur_wmodule = cur_wmodule->next;
        }
    } else
        *wmodules = cur_wmodule = calloc(1, sizeof(wmodule));

    if (!cur_wmodule) {
        if (output == NULL) {
            merror(MEM_ERROR, errno, strerror(errno));
        } else {
            snprintf(message, OS_FLSIZE + 1,
                "Could not acquire memory due to [(%d)-(%s)].",
                errno, strerror(errno));
            wm_strcat(output, message, '\n');
        }
        return (OS_INVALID);
    }

    // Get children
    if (children = OS_GetElementsbyNode(xml, node), !children) {
        if (output == NULL){
            mdebug1("Empty configuration for module '%s'.", node->element);
        }
    }

    //Policy Monitoring Module
    if (!strcmp(node->element, WM_SCA_CONTEXT.name)) {
        if (wm_sca_read(xml,children, cur_wmodule, output) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }

    OS_ClearNode(children);
    return 0;
}

#ifndef WIN32
int Read_Fluent_Forwarder(const OS_XML *xml, xml_node *node, void *d1, char **output)
{
    wmodule **wmodules = (wmodule**)d1;
    wmodule *cur_wmodule;
    xml_node **children = NULL;
    wmodule *cur_wmodule_exists;
    char message[OS_FLSIZE];

    // Allocate memory
    if ((cur_wmodule = *wmodules)) {
        cur_wmodule_exists = *wmodules;
        int found = 0;

        while (cur_wmodule_exists) {
            if(cur_wmodule_exists->tag) {
                if (!output) {
                    if(strcmp(cur_wmodule_exists->tag,node->element) == 0) {
                        cur_wmodule = cur_wmodule_exists;
                        found = 1;
                        break;
                    }
                }
            }
            cur_wmodule_exists = cur_wmodule_exists->next;
        }

        if(!found) {
            while (cur_wmodule->next)
                cur_wmodule = cur_wmodule->next;

            os_calloc(1, sizeof(wmodule), cur_wmodule->next);
            cur_wmodule = cur_wmodule->next;
        }
    } else
        *wmodules = cur_wmodule = calloc(1, sizeof(wmodule));

    if (!cur_wmodule) {
        if (output == NULL) {
            merror(MEM_ERROR, errno, strerror(errno));
        } else {
            snprintf(message, OS_FLSIZE + 1,
                "Could not acquire memory due to [(%d)-(%s)].",
                errno, strerror(errno));
            wm_strcat(output, message, '\n');
        }
        return (OS_INVALID);
    }

    // Get children
    if (children = OS_GetElementsbyNode(xml, node), !children) {
        if (output == NULL){
            mdebug1("Empty configuration for module '%s'.", node->element);
        }
    }

    // Fluent Forwarder Module
    if (!strcmp(node->element, WM_FLUENT_CONTEXT.name)) {
        if (wm_fluent_read(children, cur_wmodule) < 0) {
            OS_ClearNode(children);
            return OS_INVALID;
        }
    }

    OS_ClearNode(children);
    return 0;
}
#endif

int Test_WModule(const char *path, int type, char **output) {
    wmodule *test_wmodule;
    os_calloc(1, sizeof(wmodule), test_wmodule);

    if (ReadConfig(CWMODULE | type, path, &test_wmodule, NULL, output) < 0) {
        if (output == NULL){
            merror(CONF_READ_ERROR, "WModule");
        } else {
            wm_strcat(output, "ERROR: Invalid configuration in WModule", '\n');
        }
        wm_free(test_wmodule);
        return OS_INVALID;
    }

    wm_free(test_wmodule);
    return 0;
}
