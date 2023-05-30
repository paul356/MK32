#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_check.h"
#include "keyboard_config.h"
#include "key_definitions.h"
#include "nvs_keymaps.h"
#include "nvs_funcs.h"
#include "cJSON.h"
#include "keymap.h"
#include "hal_ble.h"
#include "function_control.h"

#define TAG "[HTTPD]"
#define ARRAY_LEN(arr) (sizeof(arr)/sizeof(arr[0]))

static time_t s_init_version;

static esp_err_t parse_http_req(httpd_req_t* req, cJSON** root)
{
    size_t buf_len = req->content_len;
    char* body = (char*)malloc(buf_len);
    if (!body) {
        return ESP_ERR_NO_MEM;
    }

    int ret = httpd_req_recv(req, body, buf_len);
    if (ret <= 0) {
        free(body);
        return ESP_ERR_INVALID_ARG;
    }

    *root = cJSON_ParseWithLength(body, buf_len);
    free(body);
    if (!root) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

static esp_err_t serve_static_files(httpd_req_t* req)
{
    extern const unsigned char index_html_start[] asm("_binary_index_html_start");
    extern const unsigned char index_html_end[] asm("_binary_index_html_end");

    extern const unsigned char app_js_start[] asm("_binary_app_js_start");
    extern const unsigned char app_js_end[] asm("_binary_app_js_end");

    extern const unsigned char style_css_start[] asm("_binary_style_css_start");
    extern const unsigned char style_css_end[] asm("_binary_style_css_end");
    
    const char* uri = req->uri;
    
    if (strcmp(uri, "/index.html") == 0 ||
        strcmp(uri, "/") == 0) {
        const size_t chunk_size = index_html_end - index_html_start;
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send_chunk(req, (const char*)index_html_start, chunk_size);
        httpd_resp_sendstr_chunk(req, NULL);
    } else if (strcmp(uri, "/app.js") == 0) {
        const size_t chunk_size = app_js_end - app_js_start;
        httpd_resp_set_type(req, "text/javascript");
        httpd_resp_send_chunk(req, (const char*)app_js_start, chunk_size);
        httpd_resp_sendstr_chunk(req, NULL);        
    } else if (strcmp(uri, "/style.css") == 0) {
        const size_t chunk_size = style_css_end - style_css_start;
        httpd_resp_set_type(req, "text/css");
        httpd_resp_send_chunk(req, (const char*)style_css_start, chunk_size);
        httpd_resp_sendstr_chunk(req, NULL);
    } else {
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_sendstr_chunk(req, "non exist URI");
        httpd_resp_sendstr_chunk(req, NULL);
    }

    return ESP_OK;
}

static esp_err_t layouts_json(httpd_req_t* req)
{
    esp_err_t err;

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{\"layouts\":{\n");
    for (int i = 0; i < layers_num; i ++) {
        httpd_resp_sendstr_chunk(req, "\"");
        httpd_resp_sendstr_chunk(req, layer_names_arr[i]);
        httpd_resp_sendstr_chunk(req, "\":[");

        for (int j = 0; j < MATRIX_ROWS; j++) {
            httpd_resp_sendstr_chunk(req, "\n  [");
            for (int k = 0; k < MATRIX_COLS; k++) {
                uint16_t key_code = keymaps[i][j][k];
                const char* keyName = GetKeyCodeName(key_code);
                if (keyName != NULL && keyName[0] != '\0') {
                    char str_buf[25];
                    snprintf(str_buf, sizeof(str_buf), "%hu", key_code);
                    err = httpd_resp_sendstr_chunk(req, str_buf);
                } else {
                    err = httpd_resp_sendstr_chunk(req, "0");
                }
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "send error, keyName is \"%s\"", keyName);
                }
                if (k != MATRIX_COLS - 1) {
                    httpd_resp_sendstr_chunk(req, ",");
                }
            }
            if (j != MATRIX_ROWS - 1) {
                httpd_resp_sendstr_chunk(req, "],");
            } else {
                httpd_resp_sendstr_chunk(req, "]");
            }
        }

        if (i != layers_num - 1) {
            httpd_resp_sendstr_chunk(req, "\n],\n");
        } else {
            httpd_resp_sendstr_chunk(req, "\n]\n");
        }
    }
    httpd_resp_sendstr_chunk(req, "}}");
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

static esp_err_t keycodes_json(httpd_req_t* req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{\"keycodes\":[\n");
    uint16_t keyCodeNum = GetKeyCodeNum();
    bool firstKey = true;
    for (uint16_t kc = 0; kc < keyCodeNum; kc++) {
        const char* keyName = GetKeyCodeName(kc);

        if (firstKey) {
            httpd_resp_sendstr_chunk(req, "  \"");
            firstKey = false;
        } else {
            httpd_resp_sendstr_chunk(req, ",\n  \"");
        }

        if (keyName != NULL && keyName[0] != '\0') {
            httpd_resp_sendstr_chunk(req, keyName);
        } else {
            httpd_resp_sendstr_chunk(req, "[ ]");
        }

        httpd_resp_sendstr_chunk(req, "\"");
    }
    httpd_resp_sendstr_chunk(req, "\n]}");
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

#define MAX_BUF_SIZE 1024
static char recv_buf[MAX_BUF_SIZE];
static esp_err_t update_keymap(httpd_req_t* req)
{
    httpd_resp_set_type(req, "text/plain");

    int total_len = req->content_len;
    if(total_len >= MAX_BUF_SIZE){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    memset(recv_buf, 0, MAX_BUF_SIZE);
    int received = 0;
    int cur_len  = 0;
    while(cur_len < total_len){
        received = httpd_req_recv(req, recv_buf + cur_len, total_len);
        if(received <= 0){
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    recv_buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(recv_buf);
    if(root == NULL){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "invalid json");
        return ESP_FAIL;
    }

    cJSON *layer = cJSON_GetObjectItem(root, "layer");
    if(layer == NULL){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "'layer' is invalid");
        return ESP_FAIL;
    }

    const char* layer_name = layer->valuestring;    
    int layer_index = -1;
    for(uint16_t i = 0; i < LAYERS; ++i){
        if(strcmp(layer_name, default_layout_names[i]) == 0) layer_index = i;
    }
    if(layer_index < 0){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "'layer' should be one of ['QWERTY', 'NUM', 'Plugins']");
        return ESP_FAIL;
    }

    cJSON *positions = cJSON_GetObjectItem(root, "positions");
    if((positions == NULL) || (cJSON_GetArraySize(positions) == 0)){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "'positions' is invalid");
        return ESP_FAIL;
    }

    cJSON *keycodes = cJSON_GetObjectItem(root, "keycodes");
    if((keycodes == NULL) || (cJSON_GetArraySize(keycodes) == 0)){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "'keycodes' is invalid");
        return ESP_FAIL;
    }

    if(cJSON_GetArraySize(positions) != cJSON_GetArraySize(keycodes)){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "length of 'positions' not match with 'keycodes'");
        return ESP_FAIL;
    }

    uint16_t temp_layer[MATRIX_ROWS * MATRIX_COLS];
    memcpy(temp_layer, keymaps[layer_index], sizeof(uint16_t) * MATRIX_ROWS * MATRIX_COLS);
    for(uint16_t i = 0; i < cJSON_GetArraySize(positions); ++i){
        uint16_t pos = (uint16_t)cJSON_GetArrayItem(positions, i)->valueint;
        uint16_t keycode = (uint16_t)cJSON_GetArrayItem(keycodes, i)->valueint;
        ESP_LOGI(TAG, "%s: pos = '%hu', keycode = '%hu'", __FUNCTION__, pos, keycode);

        if (pos >= MATRIX_ROWS * MATRIX_COLS) {
            ESP_LOGE(TAG, "%s: invalid position %hu", __FUNCTION__, pos);
            continue;
        } else if (GetKeyCodeName(keycode) == NULL) {
            ESP_LOGE(TAG, "%s: invalid keycode %hu", __FUNCTION__, keycode);
            continue;
        }

        ESP_LOGI(TAG, "%s: keycode = '%hu' -> '%hu'", __FUNCTION__, temp_layer[pos], keycode);
        temp_layer[pos] = keycode;
    }

    // save layout
    memcpy((void*)(&keymaps[layer_index][0][0]), temp_layer, sizeof(uint16_t) * MATRIX_ROWS * MATRIX_COLS);
    nvs_write_layout(layer_name, temp_layer);

    cJSON_Delete(root);

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t reset_keymap(httpd_req_t* req)
{
    httpd_resp_set_type(req, "text/plain");

    size_t buf_len = req->content_len;
    char* body = (char*)malloc(buf_len);
    if (!body) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return ESP_ERR_NO_MEM;
    }

    int ret = httpd_req_recv(req, body, buf_len);
    if (ret <= 0) {
        free(body);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return ESP_ERR_INVALID_ARG;
    }

    cJSON* root = cJSON_ParseWithLength(body, buf_len);
    free(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "invalid json");
        return ESP_ERR_NO_MEM;
    }

    cJSON* pos = cJSON_GetObjectItem(root, "positions");
    if (!pos || !cJSON_IsArray(pos) || cJSON_GetArraySize(pos) != 0) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no positions in json");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON* kc  = cJSON_GetObjectItem(root, "keycodes");
    if (!kc || !cJSON_IsArray(kc) || cJSON_GetArraySize(kc) != 0) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no keycodes in json");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON_Delete(root);

    esp_err_t err = nvs_reset_layouts();
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "reset fail");
        return ESP_FAIL;        
    }

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t upload_bin_file(httpd_req_t* req)
{
    int file_len = req->content_len;
    int accumu_len = 0;
    char str_buf[25];

    ESP_LOGI(TAG, "Starting OTA ...");

    const esp_partition_t* update_partition = NULL;

    update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no available update partition");
        return ESP_FAIL;
    }

    esp_ota_handle_t update_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
        esp_ota_abort(update_handle);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "can't begin ota");
        return ESP_FAIL;
    }

    while (accumu_len < file_len) {
        int recieved = httpd_req_recv(req, recv_buf, MAX_BUF_SIZE);
        if (recieved <= 0) {
            if (recieved == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE("HTTPD", "fail to recieve file");
            esp_ota_abort(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "fail to recieve file");
            return ESP_FAIL;
        }

        err = esp_ota_write(update_handle, recv_buf, recieved);
        if (err != ESP_OK) {
            esp_ota_abort(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "fail to write ota data");
            return ESP_FAIL;
        }

        accumu_len += recieved;
    }

    if (accumu_len != file_len) {
        esp_ota_abort(update_handle);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recieve more data than expected");
        return ESP_FAIL;
    }

    err = esp_ota_end(update_handle);
    if (err !=  ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "can't end ota successfully");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "fail to set boot partition");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/plain");
    snprintf(str_buf, sizeof(str_buf), "File size is %d. ", accumu_len);
    httpd_resp_sendstr_chunk(req, str_buf);
    httpd_resp_sendstr_chunk(req, "Please reboot keyboard ...");
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

static esp_err_t get_device_status(httpd_req_t* req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "{\n");

    // boot partition
    const esp_partition_t* curr_part = esp_ota_get_running_partition();
    httpd_resp_sendstr_chunk(req, "\"boot_partition\" : \"");
    httpd_resp_sendstr_chunk(req, curr_part->label);
    httpd_resp_sendstr_chunk(req, "\",\n");

    // ble_state
    httpd_resp_sendstr_chunk(req, "\"ble_state\" : ");
    httpd_resp_sendstr_chunk(req, is_ble_enabled() ? "true,\n" : "false,\n");

    // usb_state
    httpd_resp_sendstr_chunk(req, "\"usb_state\" : ");
    httpd_resp_sendstr_chunk(req, is_usb_enabled() ? "true,\n" : "false,\n");    

    char str_buf[25];
    snprintf(str_buf, sizeof(str_buf), "%lu", s_init_version);
    // start_time
    httpd_resp_sendstr_chunk(req, "\"init_version\" : \"");
    httpd_resp_sendstr_chunk(req, str_buf);
    httpd_resp_sendstr_chunk(req, "\"\n");

    httpd_resp_sendstr_chunk(req, "}");
    httpd_resp_sendstr_chunk(req, NULL);

    return ESP_OK;
}

static esp_err_t modify_functions(httpd_req_t* req)
{
    const char* prefix = "/api/switches/";
    const char* last_token = &req->uri[strlen(prefix)];
    esp_err_t ret;

    cJSON* root = NULL;
    esp_err_t err = parse_http_req(req, &root);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "fail to parse http request");
        return err;
    }

    if (strcmp(last_token, "WiFi") == 0) {
        cJSON_bool enabled = false;
        const char* ssid = NULL;
        const char* passwd = NULL;

        cJSON* enabledItem = cJSON_GetObjectItem(root, "enabled");
        if (enabledItem) {
            enabled = cJSON_IsTrue(enabledItem);
        }

        cJSON* ssidItem = cJSON_GetObjectItem(root, "ssid");
        if (ssidItem) {
            if (!enabledItem) enabled = true;
            ssid = cJSON_GetStringValue(ssidItem);
        }

        cJSON* passwdItem = cJSON_GetObjectItem(root, "passwd");
        if (passwdItem) {
            if (!enabledItem) enabled = true;
            passwd = cJSON_GetStringValue(passwdItem);
        }

        if (enabledItem || ssidItem || passwdItem) {
            ESP_GOTO_ON_ERROR(update_wifi_state(enabled, ssid, passwd), err_out, TAG, "fail to update wifi state");
        }
    } else if (strcmp(last_token, "BLE") == 0) {
        cJSON_bool enabled = false;
        const char* name = NULL;

        cJSON* enabledItem = cJSON_GetObjectItem(root, "enabled");
        if (enabledItem) {
            enabled = cJSON_IsTrue(enabledItem);
        }

        cJSON* nameItem = cJSON_GetObjectItem(root, "name");
        if (nameItem) {
            if (!enabledItem) enabled = true;
            name = cJSON_GetStringValue(nameItem);
        }

        if (enabledItem || nameItem) {
            ESP_GOTO_ON_ERROR(update_ble_state(enabled, name), err_out, TAG, "fail to update ble state");
        }
    } else if (strcmp(last_token, "USB") == 0) {
        cJSON_bool enabled = false;

        cJSON* enabledItem = cJSON_GetObjectItem(root, "enabled");
        if (enabledItem) {
            enabled = cJSON_IsTrue(enabledItem);

            ESP_GOTO_ON_ERROR(update_usb_state(enabled), err_out, TAG, "fail to update usb state");
        }
    } else {
        ESP_LOGE(TAG, "unkown url %s", last_token);
        ret = ESP_ERR_INVALID_ARG;
        goto err_out;
    }

    cJSON_Delete(root);
    httpd_resp_sendstr_chunk(req, NULL);    
    return ESP_OK;

err_out:
    cJSON_Delete(root);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Oops. Something is wrong.");
    return ret;
}

static struct {
    const char*    uri;
    httpd_method_t method;
    esp_err_t (*handler)(httpd_req_t* req);
    void*          user_ctx;
} handler_uris[] = {
    /* URI handler for getting uploaded files */
    {"/api/layouts",           HTTP_GET,      layouts_json,           NULL},
    {"/api/keycodes",          HTTP_GET,      keycodes_json,          NULL},
    {"/api/keymap",            HTTP_PUT,      update_keymap,          NULL},
    {"/api/keymap",            HTTP_POST,     reset_keymap,           NULL},
    {"/api/device-status",     HTTP_GET,      get_device_status,      NULL},
    {"/upload/bin_file",       HTTP_POST,     upload_bin_file,        NULL},
    {"/api/switches/*",        HTTP_PUT,      modify_functions,       NULL},
    {"/*",                     HTTP_GET,      serve_static_files,     NULL},
};

esp_err_t start_file_server()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    s_init_version = esp_random();

    // match uri by strncmp
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    for (int i = 0; i < ARRAY_LEN(handler_uris); i++) {
        httpd_uri_t uri = {
            .uri      = handler_uris[i].uri,
            .method   = handler_uris[i].method,
            .handler  = handler_uris[i].handler,
            .user_ctx = handler_uris[i].user_ctx
        };
        httpd_register_uri_handler(server, &uri);
    }

    return ESP_OK;
}
