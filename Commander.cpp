#include "Commander.hpp"
#include "NvsSettings.hpp"
#include "TelnetServer.hpp"

extern TelnetServer telnet_info;
extern TelnetServer telnet_uart2;

//=============================================================================
// commands
//=============================================================================
//sys
static void sys_bootAP(WiFiClient&, String);
static void sys_reboot(WiFiClient&, String);
static void sys_erase_all(WiFiClient&, String);
//wifi
static void wifi_list(WiFiClient&, String);
static void wifi_add(WiFiClient&, String);
static void wifi_erase(WiFiClient&, String);
//net
static void net_hostname(WiFiClient&, String);
static void net_APname(WiFiClient&, String);
static void net_mac(WiFiClient&, String);
static void net_servers(WiFiClient&, String);
//uart2
static void uart2_baud(WiFiClient&, String);

//=============================================================================
// command list - name:function
//=============================================================================
using cmdfunc_t = void(*)(WiFiClient&, String);
using cmd_t = struct {
    const char* cmd;
    cmdfunc_t func;
    const char* help;
};

static cmd_t commands[] = {
        //name          function        help
        { "?",          NULL,           "?                                  :alias for help" },
        { "help",       NULL,           "help                               :you are here" },
        { "bye",        NULL,           "bye                                :close this connection" },
        { "sys",        NULL,           NULL },
        {   "bootAP",   sys_bootAP,     "sys <bootAP | bootAP=0 | bootAP=1> :view or set boot flag" },
        {   "reboot",   sys_reboot,     "sys reboot                         :reset esp32" },
        {   "erase all",sys_erase_all,  "sys erase all                      :erase all stored settings" },

        { "wifi",       NULL,           NULL },
        {   "list",     wifi_list,      "wifi list                          :list all stored wifi connections" },
        {   "add",      wifi_add,       "wifi add # <ssid=name | pass=pw>   :add ssid or password for index# 0-7" },
        {   "erase",    wifi_erase,     "wifi erase #                       :erase stored wifi info in index# 0-7" },

        { "net",        NULL,           NULL },
        {   "hostname", net_hostname,   "net <hostname | hostname=myname>   :view or set hostname" },
        {   "APname",   net_APname,     "net <APname | APname=myapname>     :view or set access point name" },
        {   "mac",      net_mac,        "net mac                            :view mac address" },
        {   "servers",  net_servers,    "net servers                        :view telnet server status" },

        { "uart2",      NULL,           NULL },
        {   "baud",     uart2_baud,     "uart2 <baud | baud=115200>         :view or set uart2 baudrate" },

        { NULL,         NULL }              //end of table
};

//=============================================================================
// common print functions
//=============================================================================
void help(WiFiClient& client)
{
    client.printf("\navailable commands:\n\n");
    for(auto i = 0; commands[i].cmd != NULL; i++){
        if(commands[i].help) client.printf("%s\n", commands[i].help);
    }
    client.printf("\n");
}
void bad(WiFiClient& client){ client.printf("unknown command\n"); }

//=============================================================================
// process incoming command passed from telnet function (already trimmed)
//=============================================================================
void Commander::process(WiFiClient& client, String s)
{
    //can 'logoff' with bye
    if(s == "bye"){
        telnet_info.stop_client();
        return;
    }
    //check for root command
    for(auto i = 0; commands[i].func || commands[i].cmd; i++){
        if(commands[i].func) continue; //only looking for root command
        if(not s.startsWith(commands[i].cmd)) continue; //no match
        //root command found
        s.replace(commands[i].cmd, ""); //remove command string
        if(s[0] != ' '){ help(client); return;  } //no space after command
        s.trim();
        if(not s[0]){ help(client); return; } //unfinshed command
        //check for sub command
        for(i++; commands[i].func; i++){
            if(not s.startsWith(commands[i].cmd)) continue;
            //sub command found
            s.replace(commands[i].cmd, "");
            s.trim();
            //run command
            commands[i].func(client, s);
            return;
        }
        //bad subcommand
        help(client);
    }

    bad(client);
}

//=============================================================================
// all command functions
//=============================================================================
//sys bootAP
static void sys_bootAP(WiFiClient& client, String s)
{
    //no args
    if(not s[0]){
        NvsSettings settings;
        client.printf("bootAP: %s\n", settings.boot_to_AP() ? "true" : "false");
    }
    //=1
    else if(s.startsWith("=1")){
        NvsSettings settings;
        settings.boot_to_AP(true);
    }
    //=0
    else if(s.startsWith("=0")){
        NvsSettings settings;
        settings.boot_to_AP(false);
    }
    else help(client);
}
//sys reboot
static void sys_reboot(WiFiClient& client, String s)
{
    if(s[0]){ bad(client); return; }
    client.printf("rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
}
//sys erase all
static void sys_erase_all(WiFiClient& client, String s)
{
    if(s[0]){ bad(client); return; }
    client.printf("erasing all stored data...");
    NvsSettings settings;
    settings.erase_all();
    client.printf("done.\n");
}
//wifi list
static void wifi_list(WiFiClient& client, String s)
{
    if(s[0]){ bad(client); return; }
    NvsSettings settings;
    int max = settings.wifimaxn();
    int maxslen = 0;
    int maxplen = 0;
    //get max len of chars for each field
    for( auto i = 0; i < max; i++ ){
        int s = settings.ssid(i).length();
        int p = settings.pass(i).length();
        if(s > maxslen) maxslen = s;
        if(p > maxplen) maxplen = p;
    }
    String fmt{"  #  %-" + String(maxslen) + "s  %-" + String(maxplen) + "s \n"};
    client.printf(fmt.c_str(),"SSID","PASS");
    fmt = " %2d  %-" + String(maxslen) + "s  %-" + String(maxplen) + "s \n";
    for(auto i = maxslen + maxplen + 10; i; client.printf("-"), i--);
    client.printf("\n");
    for( auto i = 0; i < max; i++ ){
        client.printf(fmt.c_str(),
            i, settings.ssid(i).c_str(), settings.pass(i).c_str()
        );
    }
}
//wifi add
static void wifi_add(WiFiClient& client, String s)
{
    //"0 ssid=myssid"
    NvsSettings settings;
    //"0 ssid=myssid"
    //"0 pass=mypass"
    int idx = 0;
    if(s.substring(0,2) != "0 "){
        idx = s.toInt();
        if(idx == 0){
            client.printf("missing index# or index# not valid\n");
            return;
        }
    }
    if(idx > settings.wifimaxn()){
        client.printf("index# is out of range (max %s)\n", settings.wifimaxn());
        return;
    }
    int si = s.indexOf("ssid=");
    if(si > 0){
        settings.ssid(idx, &s[si+5]);
        return;
    }
    si = s.indexOf("pass=");
    if(si > 0){
        settings.pass(idx, &s[si+5]);
        return;
    }
    //bad command
    help(client);
}
//wifi erase
static void wifi_erase(WiFiClient& client, String s)
{
    //"wifi erase 0"
    int idx = 0;
    if(s != "0"){
        idx = s.toInt();
        if(idx == 0){
            client.printf("missing index# or index# not valid\n");
            return;
        }
    }
    NvsSettings settings;
    if(idx > settings.wifimaxn()){
        client.printf("index# is out of range\n");
        return;
    }
    settings.ssid(idx, "");
    settings.pass(idx, "");
}
//net hostname
static void net_hostname(WiFiClient& client, String s)
{
    //no arg
    if(not s[0]){
        NvsSettings settings;
        client.printf("hardware: %s  stored: %s\n",
            WiFi.getHostname(), settings.hostname().c_str()
        );
        return;
    }
    //=myname
    if(s[0] == '='){
        NvsSettings settings;
        s = s.substring(1);
        if(s.length() > 32){
            client.printf("hostname too long (32 chars max)\n");
            return;
        }
        settings.hostname(s); //nvs storage
        WiFi.setHostname(s.c_str()); //and out to tcpip (may not see until reboot)
        return;
    }
    //bad command
    help(client);
}
//net APname
static void net_APname(WiFiClient& client, String s)
{
    //no arg
    if(not s[0]){
        NvsSettings settings;
        client.printf("%s\n", settings.APname().c_str());
        return;
    }
    //=myAPname
    if(s[0] == '='){
        NvsSettings settings;
        s = s.substring(1);
        if(s.length() > 32){
            client.printf("APname too long (32 chars max)\n");
            return;
        }
        settings.APname(s); //nvs storage
        return;
    }
    //bad command
    help(client);
}
//net mac
static void net_mac(WiFiClient& client, String s)
{
    //no arg
    if(not s[0]){
        client.printf("%s\n", WiFi.macAddress().c_str());
        return;
    }
    //bad command
    help(client);
}

//net servers
static void net_servers(WiFiClient& client, String s)
{
    //no arg
    if(not s[0]){
        telnet_info.status(client);
        telnet_uart2.status(client);
        return;
    }
    //bad command
    help(client);
}

//uart2 baud
void uart2_baud(WiFiClient& client, String s)
{
    //no arg
    if(not s[0]){
        NvsSettings settings;
        client.printf("uart2 baud: %d\n", settings.uart2baud());
        return;
    }
    //"=115200"
    if(s[0] == '='){
        s = s.substring(1);
        int baud = s.toInt();
        if(baud == 0){
            client.printf("baud value not valid\n");
            return;
        }
        NvsSettings settings;
        settings.uart2baud(baud);
        return;
    }
    //bad command
    help(client);
}
