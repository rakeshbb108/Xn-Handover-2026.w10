#ifndef CREATE_MPLANE_XML_CONFIG_H
#define CREATE_MPLANE_XML_CONFIG_H

#include "../ru-mplane-api.h"
#include "radio/COMMON/common_lib.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

bool configure_ru_from_xml(ru_mplane_config_t *ru_config, char **xml_string);

#endif /* CREATE_MPLANE_XML_CONFIG_H */