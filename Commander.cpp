#include "Commander.hpp"
#include "WifiCredentials.hpp"

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
        { "sys",        NULL,           NULL },
        {   "boot",     sys_boot,       "boot | boot=AP | boot=STA" },
        {   "reboot",   sys_reboot,     NULL },
        {   "erase all",sys_erase_all,  NULL   },

        { "wifi",       NULL,           NULL },
        {   "list",     wifi_list,      NULL },
        {   "add",      wifi_add,       "add # ssid=ssidname | add # pass=password" },
        {   "erase",    wifi_erase,     "erase #" },

        { "net",        NULL,           NULL },
        {   "hostname", net_hostname,   "hostname | hostname=myname" },
        {   "APname",   net_APname,     "APname | APname=myapname" },
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
        WifiCredentials wifidata;
        client.printf("boot: %s\n", wifidata.boot() ? "AP" : "STA");
    }
    //=AP
    else if(s.startsWith("=AP")){
        WifiCredentials wifidata;
        wifidata.boot(wifidata.AP);
    }
    //=STA
    else if(s.startsWith("=STA")){
        WifiCredentials wifidata;
        wifidata.boot(wifidata.STA);
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
    WifiCredentials wifidata;
    wifidata.erase_all();
    client.printf("done.\n");
}
//wifi list
static void wifi_list(WiFiClient& client, String s)
{
    if(s[0]){ bad(client); return; }
    WifiCredentials wifidata;
    int max = wifidata.maxn();
    int maxslen = 0;
    int maxplen = 0;
    //get max len of chars for each field
    for( auto i = 0; i < max; i++ ){
        int s = wifidata.ssid(i).length();
        int p = wifidata.pass(i).length();
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
            i, wifidata.ssid(i).c_str(), wifidata.pass(i).c_str()
        );
    }
}
//wifi add
static void wifi_add(WiFiClient& client, String s)
{
    //"0 ssid=myssid"
    WifiCredentials wifidata;
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
    if(idx > wifidata.maxn()){
        client.printf("index# is out of range (max %s)\n", wifidata.maxn());
        return;
    }
    int si = s.indexOf("ssid=");
    if(si > 0){
        wifidata.ssid(idx, &s[si+5]);
        return;
    }
    si = s.indexOf("pass=");
    if(si > 0){
        wifidata.pass(idx, &s[si+5]);
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
    WifiCredentials wifidata;
    if(idx > wifidata.maxn()){
        client.printf("index# is out of range\n");
        return;
    }
    wifidata.ssid(idx, "");
    wifidata.pass(idx, "");
}
//net hostname
static void net_hostname(WiFiClient& client, String s)
{
    //no arg
    if(not s[0]){
        WifiCredentials wifidata;
        client.printf("hardware: %s  stored: %s\n",
            WiFi.getHostname(), wifidata.hostname().c_str()
        );
        return;
    }
    //=myname
    if(s[0] == '='){
        WifiCredentials wifidata;
        s = s.substring(1);
        if(s.length() > 32){
            client.printf("hostname too long (32 chars max)\n");
            return;
        }
        wifidata.hostname(s); //nvs storage
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
        WifiCredentials wifidata;
        client.printf("%s\n", wifidata.APname().c_str());
        return;
    }
    //=myAPname
    if(s[0] == '='){
        WifiCredentials wifidata;
        s = s.substring(1);
        if(s.length() > 32){
            client.printf("APname too long (32 chars max)\n");
            return;
        }
        wifidata.APname(s); //nvs storage
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

