#include "create-xml-config.h"
#include "common/utils/nr/nr_common.h"

void add_element(xmlNodePtr parent, const char *name, const char *content) {
    xmlNewChild(parent, NULL, BAD_CAST name, BAD_CAST content);
}

bool configure_ru_from_xml(ru_mplane_config_t *ru_config, char **xml_string) {
    // Create a new XML document
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "user-plane-configuration");
    xmlNewProp(root_node, BAD_CAST "xmlns", BAD_CAST "urn:o-ran:uplane-conf:1.0");
    xmlDocSetRootElement(doc, root_node);

    // Create tx-array-carriers node
    xmlNodePtr tx_node = xmlNewChild(root_node, NULL, BAD_CAST "tx-array-carriers", NULL);
    add_element(tx_node, "name", ru_config->tx_array_carrier.name);

    char buffer[50];
    sprintf(buffer, "%d", ru_config->tx_array_carrier.arfcn_center);
    add_element(tx_node, "absolute-frequency-center", buffer);

    sprintf(buffer, "%ld", ru_config->tx_array_carrier.center_channel_bw);
    add_element(tx_node, "center-of-channel-bandwidth", buffer);

    sprintf(buffer, "%d", ru_config->tx_array_carrier.channel_bw);
    add_element(tx_node, "channel-bandwidth", buffer);

    add_element(tx_node, "active", ru_config->tx_array_carrier.ru_carrier);
    add_element(tx_node, "rw-duplex-scheme", ru_config->tx_array_carrier.rw_duplex_scheme);
    add_element(tx_node, "rw-type", ru_config->tx_array_carrier.rw_type);

    sprintf(buffer, "%.1f", ru_config->tx_array_carrier.gain);
    add_element(tx_node, "gain", buffer);

    sprintf(buffer, "%d", ru_config->tx_array_carrier.dl_radio_frame_offset);
    add_element(tx_node, "downlink-radio-frame-offset", buffer);

    sprintf(buffer, "%d", ru_config->tx_array_carrier.dl_sfn_offset);
    add_element(tx_node, "downlink-sfn-offset", buffer);

    // Create rx-array-carriers node
    xmlNodePtr rx_node = xmlNewChild(root_node, NULL, BAD_CAST "rx-array-carriers", NULL);
     add_element(rx_node, "name", ru_config->rx_array_carrier.name);

    sprintf(buffer, "%d", ru_config->rx_array_carrier.arfcn_center);
    add_element(rx_node, "absolute-frequency-center", buffer);

    sprintf(buffer, "%ld", ru_config->rx_array_carrier.center_channel_bw);
    add_element(rx_node, "center-of-channel-bandwidth", buffer);

    sprintf(buffer, "%d", ru_config->rx_array_carrier.channel_bw);
    add_element(rx_node, "channel-bandwidth", buffer);

    add_element(rx_node, "active", ru_config->rx_array_carrier.ru_carrier);

    sprintf(buffer, "%d", ru_config->rx_array_carrier.dl_radio_frame_offset);
    add_element(rx_node, "downlink-radio-frame-offset", buffer);

    sprintf(buffer, "%d", ru_config->rx_array_carrier.dl_sfn_offset);
    add_element(rx_node, "downlink-sfn-offset", buffer);

    sprintf(buffer, "%.1f", ru_config->rx_array_carrier.gain_correction);
    add_element(rx_node, "gain-correction", buffer);

    sprintf(buffer, "%d", ru_config->rx_array_carrier.n_ta_offset);
    add_element(rx_node, "n-ta-offset", buffer);

    // Save the XML document to a string
    xmlChar *xml_buffer = NULL;
    int buffer_size = 0;
    xmlDocDumpFormatMemoryEnc(doc, &xml_buffer, &buffer_size, "UTF-8", 1);

    // Convert xmlChar* to a regular C string
    *xml_string = strdup((const char *)xml_buffer);

    // Free the document and the temporary XML buffer
    xmlFree(xml_buffer);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return true;
}
