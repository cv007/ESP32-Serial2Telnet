#include "Commander.hpp"
#include "NvsSettings.hpp"
#include "TelnetServer.hpp"

extern TelnetServer telnet_info;

//=============================================================================
// commands
//=============================================================================
//sys
static void sys_boot(WiFiClient&, String);
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

//=============================================================================
// command list - name:function
//=============================================================================
typedef void(*cmdfunc_t)(WiFiClient&, String);
typedef struct {
    const char* cmd;
    cmdfunc_t func;
    const char* help;
} cmd_t;

static cmd_t commands[] = {
        { "?",          NULL,           NULL }, //will force help
        { "help",       NULL,           NULL }, //will force help
        { "sys",        NULL,           NULL },
        {   "boot",     sys_boot,       "<boot | boot=AP | boot=STA>" },
        {   "reboot",   sys_reboot,     NULL },
        {   "erase all",sys_erase_all,  NULL },

        { "wifi",       NULL,           NULL },
        {   "list",     wifi_list,      NULL },
        {   "add",      wifi_add,       "add # <ssid=ssidname | pass=password>" },
        {   "erase",    wifi_erase,     "erase #" },

        { "net",        NULL,           NULL },
        {   "hostname", net_hostname,   "<hostname | hostname=myname>" },
        {   "APname",   net_APname,     "<APname | APname=myapname>" },
        {   "mac",      net_mac,        NULL },

        { NULL,         NULL }              //end of table
};

//=============================================================================
// common print functions
//=============================================================================
void help(WiFiClient& client)
{
    client.printf("available commands:\n");
    for(auto i = 0, ii = 0; commands[i].cmd != NULL; i++){
        if(commands[i].func == NULL){
            ii = i; //save root command index
            continue; //don't print root command
        }
        client.printf("  %s %s\n",
            commands[ii].cmd, commands[i].help ? commands[i].help : commands[i].cmd
        );
    }
}
void bad(WiFiClient& client){ client.printf("unknown command\n\n"); }

//=============================================================================
// process incoming command passed from telnet function (already trimmed)
//=============================================================================
void Commander::process(WiFiClient& client, String s)
{
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
//sys boot
static void sys_boot(WiFiClient& client, String s)
{
    //no args
    if(not s[0]){
        NvsSettings settings;
        client.printf("boot: %s\n", settings.boot() ? "AP" : "STA");
    }
    //=AP
    else if(s.startsWith("=AP")){
        NvsSettings settings;
        settings.boot(settings.AP);
    }
    //=STA
    else if(s.startsWith("=STA")){
        NvsSettings settings;
        settings.boot(settings.STA);
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
    help(client);
}
//wifi erase
static void wifi_erase(WiFiClient& client, String s)
{
    if(s[0] != ' '){ help(client); return; }
    s.trim();
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
    help(client);
}
//net mac
static void net_mac(WiFiClient& client, String s)
{
    if(s[0]){ help(client); return;  }
    client.printf("%s\n", WiFi.macAddress().c_str());
}

