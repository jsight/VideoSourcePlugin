#pragma once

#include "OBSApi.h"
#include "vlc.h"

#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <propsys.h>
#include <Functiondiscoverykeys_devpkey.h>

#include <tuple>
#include <vector>

struct AudioOutputDevice 
{

private:
    String name;
    String longName;
    int index;

public:
    AudioOutputDevice(String name, String longName)
    {
        this->name = name;
        this->longName = longName;
    }

public:
    String &GetName() { return name; }
    String &GetLongName() { return longName; }
};

class AudioOutputType 
{

private:
    String name;
    String description;
    std::vector<AudioOutputDevice> audioOutputDevices;
    bool isEnabled;

public:
    AudioOutputType(String name, String description) 
    {
        this->name = name;
        this->description = description;
    }
    
public:
    void AddAudioOutputDevice(AudioOutputDevice &audioOutputDevice) 
    {
        audioOutputDevices.push_back(audioOutputDevice); 
    }

public:
    void SetName(String &name) { this->name = name; }
    String &GetName() { return name; }
    String &GetDescription() { return description; }

    std::vector<AudioOutputDevice> &GetAudioOutputDevices() { return audioOutputDevices; }
};

class VideoSourceConfig 
{

private:
    XElement *element;

    std::vector<AudioOutputType> audioOutputTypes;

public:
    String pathOrUrl;
    unsigned int width;
    unsigned int height;
    bool isStretching;
    unsigned int volume;
    bool isAudioOutputToStream;
    String audioOutputType;
    String audioOutputDevice;

public:
    VideoSourceConfig(XElement *element)
    {
        this->element = element;
        Reload();
    }

    ~VideoSourceConfig()
    {
    }

public:
    

    void Populate()
    {
        pathOrUrl = TEXT("");
        width = 640;
        height = 480;
        volume = 100;
        isStretching = false;
        isAudioOutputToStream = true;
        audioOutputType = TEXT("wavemapper");
        audioOutputDevice = TEXT("");
    }

    void Reload()
    {
        pathOrUrl = element->GetString(TEXT("pathOrUrl"));
        width = element->GetInt(TEXT("width"));
        height = element->GetInt(TEXT("height"));
        volume = element->GetInt(TEXT("volume"));
        isStretching = element->GetInt(TEXT("isStretching")) == 1;
        isAudioOutputToStream = element->GetInt(TEXT("isAudioOutputToStream")) == 1;
        audioOutputType = element->GetString(TEXT("audioOutputType"));
        audioOutputDevice = element->GetString(TEXT("audioOutputDevice"));
    }

    void Save()
    {
        element->SetString(TEXT("pathOrUrl"), pathOrUrl);
        element->SetInt(TEXT("width"), width);
        element->SetInt(TEXT("height"), height);
        element->SetInt(TEXT("volume"), volume);
        element->SetInt(TEXT("isStretching"), isStretching ? 1 : 0);
        element->SetInt(TEXT("isAudioOutputToStream"), isAudioOutputToStream ? 1 : 0);
        element->SetString(TEXT("audioOutputType"), audioOutputType);
        element->SetString(TEXT("audioOutputDevice"), audioOutputDevice);
    }

    void InitializeAudioOutputVectors(libvlc_instance_t *vlc) {
        audioOutputTypes.clear();
        
        libvlc_audio_output_t *typeList = libvlc_audio_output_list_get(vlc);
        libvlc_audio_output_t *typeListNode = typeList;
        while(typeListNode) {
            AudioOutputType audioOutputType(typeListNode->psz_name, typeListNode->psz_description);
#ifdef VLC21
            if (audioOutputType.GetName() == "waveout" || audioOutputType.GetName() == "mmdevice" || audioOutputType.GetName() == "directsound" || audioOutputType.GetName() == "adummy") {
                libvlc_audio_output_device_t *deviceList = libvlc_audio_output_device_list_get(vlc, typeListNode->psz_name);
                libvlc_audio_output_device_t *deviceListNode = deviceList;
                while(deviceListNode) {
                    audioOutputType.AddAudioOutputDevice(AudioOutputDevice(deviceListNode->psz_description, deviceListNode->psz_device));
                    deviceListNode = deviceListNode->p_next;
                }
                libvlc_audio_output_device_list_release(deviceList);
            }
#else
            if (audioOutputType.GetName() == "waveout" || audioOutputType.GetName() == "adummy" || audioOutputType.GetName() == "aout_directx") {
                char *device = typeListNode->psz_name;

                // fix directx reporting wrong device name
                if (audioOutputType.GetName() == "aout_directx") {
                    audioOutputType.SetName(String("directx"));
                    device = "directx";
                }

                for(int i = 0; i < libvlc_audio_output_device_count(vlc, device); i++) {
                    char *longName = libvlc_audio_output_device_longname(vlc, device, i);
                    audioOutputType.AddAudioOutputDevice(AudioOutputDevice(longName, longName));
                    libvlc_free(longName);
                }
                audioOutputTypes.push_back(audioOutputType);
            }
#endif
            typeListNode = typeListNode->p_next;
        }
        libvlc_audio_output_list_release(typeList);
                
    }

    std::vector<AudioOutputType> &GetAudioOutputTypes()
    {
        return audioOutputTypes;
    }

    AudioOutputType &GetAudioOutputType(String name)
    {
        for(auto i = audioOutputTypes.begin(); i < audioOutputTypes.end(); i++)
        {
            AudioOutputType &type = *i;
            if ((*i).GetName() == name) {
                return type;
            }
        }

        // device not found, return wavemapper
        // how could this happen? device disappeared/unplugged
        assert(audioOutputTypes.size());
        return audioOutputTypes[0];
    }
};