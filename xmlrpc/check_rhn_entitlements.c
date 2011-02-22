/**
 * Monitoring Plugin - check_rhn_entitlements
 **
 *
 * check_timeout - Check available entitlements.
 * Copyright (C) 2010 Marius Rieder <marius.rieder@durchmesser.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 */

const char *progname  = "check_rhn_entitlements";
const char *progvers  = "0.1";
const char *progcopy  = "2011";
const char *progauth = "Marius Rieder <marius.rieder@durchmesser.ch>";
const char *progusage = "[-t <timeout>]";

#include "mp_common.h"
#include "mp_eopt.h"
#include "xmlrpc_utils.h"

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xmlrpc.h>
#include <xmlrpc_client.h>

const char *url = NULL;
const char *user = NULL;
const char *pass = NULL;
char **channel = NULL;
char **system_name = NULL;
int channels = 0;
int systems = 0;
thresholds *free_thresholds = NULL;

int main (int argc, char **argv) {
    /* Local variables */
    xmlrpc_env env;
    xmlrpc_value *result;
    xmlrpc_value *array;
    xmlrpc_value *item;
    xmlrpc_value *value;
    char *out = NULL;
    char *label;
    char *name;
    char *key;
    int used_slots;
    int total_slots;
    int size, state=0, i, j;

    /* Set signal handling and alarm */
    if (signal (SIGALRM, timeout_alarm_handler) == SIG_ERR)
        exit(STATE_CRITICAL);

    /* Set Default range */
    setWarn(&free_thresholds, "10:",0);
    setCrit(&free_thresholds, "5:",0);

    if (process_arguments (argc, argv) == 1)
        exit(STATE_CRITICAL);

    alarm(mp_timeout);

    /* Create XMLRPC env */
    env = mp_xmlrpc_init();

    /* RHN Login */
    result = xmlrpc_client_call(&env, url, "auth.login", "(ss)", user, pass);
    unknown_if_xmlrpc_fault(&env);

    xmlrpc_parse_value(&env, result, "s", &key);

    if (mp_verbose >= 2) {
        printf("Login Successfull: %s\n", key);
    }

    /* Get RHN Entitlements */
    result = xmlrpc_client_call(&env, url, "satellite.listEntitlements", "(s)",
            key);
    unknown_if_xmlrpc_fault(&env);

    if (mp_verbose >= 2) {
        printf("Get Entitlements successfully\n");
    }

    if(xmlrpc_value_type(result) != XMLRPC_TYPE_STRUCT)
        unknown("Unknow return value.");

    /* Check system entitlements */
    if (system_name) {
        xmlrpc_struct_read_value(&env, result, "system", &array);
        unknown_if_xmlrpc_fault(&env);

        size = xmlrpc_array_size(&env, array);
        unknown_if_xmlrpc_fault(&env);

        for(i=0; i < size; i++) {
            xmlrpc_array_read_item(&env, array, i, &item);
            unknown_if_xmlrpc_fault(&env);

            xmlrpc_struct_read_value(&env, item, "label", &value);
            unknown_if_xmlrpc_fault(&env);

            xmlrpc_parse_value(&env, value, "s", &label);
            unknown_if_xmlrpc_fault(&env);

            if (mp_verbose >= 2) {
                printf("systems => %s\n", label);
            }

            for(j=0; j < systems; j++) {
                if (strcmp(system_name[j], label) == 0) {
                    // Read Name
                    xmlrpc_struct_read_value(&env, item, "name", &value);
                    unknown_if_xmlrpc_fault(&env);
                    xmlrpc_parse_value(&env, value, "s", &name);
                    unknown_if_xmlrpc_fault(&env);

                    if (mp_verbose >= 2) {
                        printf(" name => %s\n", name);
                    }

                    // Read Free
                    xmlrpc_struct_read_value(&env, item, "used_slots", &value);
                    unknown_if_xmlrpc_fault(&env);
                    xmlrpc_parse_value(&env, value, "i", &used_slots);
                    unknown_if_xmlrpc_fault(&env);

                    if (mp_verbose >= 2) {
                        printf(" used_slots => %d\n", used_slots);
                    }

                    // Read Total
                    xmlrpc_struct_read_value(&env, item, "total_slots", &value);
                    unknown_if_xmlrpc_fault(&env);
                    xmlrpc_parse_value(&env, value, "i", &total_slots);
                    unknown_if_xmlrpc_fault(&env);

                    if (mp_verbose >= 2) {
                        printf(" total_slots => %d\n", total_slots);
                    }

                    char *tmp;
                    tmp = malloc(128);
                    snprintf(tmp, 128, "%s (%d/%d)", name, used_slots, total_slots);
                    mp_strcat_space(&out, tmp);
                    free(tmp);

                    perfdata_int(label, used_slots, "",
                            (total_slots - free_thresholds->warning->start),
                            (total_slots - free_thresholds->critical->start),
                            0, total_slots);

                    if (state < get_status((total_slots-used_slots),free_thresholds)) {
                        state = get_status((total_slots-used_slots),free_thresholds);
                    }
                }
            }
        }
    }

    /* Check channel entitlements */
    if (channel) {
        xmlrpc_struct_read_value(&env, result, "channel", &array);
        unknown_if_xmlrpc_fault(&env);

        size = xmlrpc_array_size(&env, array);
        unknown_if_xmlrpc_fault(&env);

        for(i=0; i < size; i++) {
            xmlrpc_array_read_item(&env, array, i, &item);
            unknown_if_xmlrpc_fault(&env);

            xmlrpc_struct_read_value(&env, item, "label", &value);
            unknown_if_xmlrpc_fault(&env);

            xmlrpc_parse_value(&env, value, "s", &label);
            unknown_if_xmlrpc_fault(&env);

            if (mp_verbose >= 2) {
                printf("channel => %s\n", label);
            }

            for(j=0; j < channels; j++) {
                if (strcmp(channel[j], label) == 0) {
                    // Read Name
                    xmlrpc_struct_read_value(&env, item, "name", &value);
                    unknown_if_xmlrpc_fault(&env);
                    xmlrpc_parse_value(&env, value, "s", &name);
                    unknown_if_xmlrpc_fault(&env);

                    if (mp_verbose >= 2) {
                        printf(" name => %s\n", name);
                    }

                    // Read Free
                    xmlrpc_struct_read_value(&env, item, "used_slots", &value);
                    unknown_if_xmlrpc_fault(&env);
                    xmlrpc_parse_value(&env, value, "i", &used_slots);
                    unknown_if_xmlrpc_fault(&env);

                    if (mp_verbose >= 2) {
                        printf(" used_slots => %d\n", used_slots);
                    }

                    // Read Total
                    xmlrpc_struct_read_value(&env, item, "total_slots", &value);
                    unknown_if_xmlrpc_fault(&env);
                    xmlrpc_parse_value(&env, value, "i", &total_slots);
                    unknown_if_xmlrpc_fault(&env);

                    if (mp_verbose >= 2) {
                        printf(" total_slots => %d\n", total_slots);
                    }

                    char *tmp;
                    tmp = malloc(128);
                    snprintf(tmp, 128, "%s (%d/%d)", name, used_slots, total_slots);
                    mp_strcat_space(&out, tmp);
                    free(tmp);

                    perfdata_int(label, used_slots, "",
                            (total_slots - free_thresholds->warning->start),
                            (total_slots - free_thresholds->critical->start),
                            0, total_slots);

                    if (state < get_status((total_slots-used_slots),free_thresholds)) {
                        state = get_status((total_slots-used_slots),free_thresholds);
                    }
                }
            }
        }
    }

    switch ( state ) {
        case STATE_OK:
            ok(out);
        case STATE_WARNING:
            warning(out);
        case STATE_CRITICAL:
            critical(out);
    }

    unknown(out);
}

int process_arguments (int argc, char **argv) {
    int c;
    int option = 0;

    static struct option longopts[] = {
        MP_LONGOPTS_DEFAULT,
        MP_LONGOPTS_TIMEOUT,
        MP_LONGOPTS_EOPT,
        {"url", required_argument, NULL, (int)'U'},
        {"user", required_argument, NULL, (int)'u'},
        {"pass", required_argument, NULL, (int)'p'},
        {"channel", required_argument, NULL, (int)'C'},
        {"system", required_argument, NULL, (int)'S'},
        MP_LONGOPTS_END
    };

    while (1) {
        c = getopt_long (argc, argv, MP_OPTSTR_DEFAULT"E::t:U:u:p:C:S:w:c:", longopts, &option);

        if (c == -1 || c == EOF)
            break;

        switch (c) {
            /* Default opts */
            case 'h':
                print_help();
                exit(0);
            case 'V':
                print_revision();
                exit (0);
            case 'v':
                mp_verbose++;
                break;
            /* Local opts */
            case 'U':
                url = optarg;
                break;
            case 'u':
                user = optarg;
                break;
            case 'p':
                pass = optarg;
                break;
            case 'C':
                channel = realloc(channel, sizeof(char*)*(channels+1));
                channel[channels] = optarg;
                channels++;
                break;
            case 'S':
                system_name = realloc(system_name, sizeof(char*)*(systems+1));
                system_name[systems] = optarg;
                systems++;
                break;
            /* EOPT opt */
            case 'E':
                argv = mp_eopt(&argc, argv, optarg);
                break;
            /* Timeout opt */
            case 't':
                getopt_timeout(optarg);
                break;
        }

        getopt_wc(c, optarg, &free_thresholds);
        getopt_default(c);

    }

    if (!url)
        usage("A URL is mandatory.");
    if (!user)
        usage("A Username is mandatory.");
    if (!pass)
        usage("A Password is mandatory.");
    if (!channel && !system_name)
        usage("A Channel or System entitlement to check is mandatory.");

    return(OK);
}

void print_help (void) {
    print_revision();
    print_copyright();

    printf("\n");

    printf("This plugin check RHN or a satellite for available entitlements.");

    printf("\n\n");

    print_usage();

    print_help_default();

    printf(" -U, --url=URL\n");
    printf("      URL of the RHN XML-RPC api.\n");
    printf(" -u, --user=USER\n");
    printf("      USER to log in.\n");
    printf(" -p, --pass=PASS\n");
    printf("      PASS to log in.\n");
    printf(" -C, --channel=CHANNEL\n");
    printf("      CHANNEL entitlement to check.\n");
    printf(" -S, --system=SYSTEM\n");
    printf("      SYSTEM  entitlement to check.\n");

    print_help_timeout();
    print_help_eopt();
}

