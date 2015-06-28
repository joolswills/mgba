/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "config.h"

#include "util/formatting.h"
#include "util/string.h"
#include "util/vfs.h"

#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <strsafe.h>
#endif

#define SECTION_NAME_MAX 128

static const char* _lookupValue(const struct GBAConfig* config, const char* key) {
	const char* value;
	if (config->port) {
		value = ConfigurationGetValue(&config->configTable, config->port, key);
		if (value) {
			return value;
		}
	}
	value = ConfigurationGetValue(&config->configTable, 0, key);
	if (value) {
		return value;
	}
	if (config->port) {
		value = ConfigurationGetValue(&config->defaultsTable, config->port, key);
		if (value) {
			return value;
		}
	}
	return ConfigurationGetValue(&config->defaultsTable, 0, key);
}

static bool _lookupCharValue(const struct GBAConfig* config, const char* key, char** out) {
	const char* value = _lookupValue(config, key);
	if (!value) {
		return false;
	}
	if (*out) {
		free(*out);
	}
	*out = strdup(value);
	return true;
}

static bool _lookupIntValue(const struct GBAConfig* config, const char* key, int* out) {
	const char* charValue = _lookupValue(config, key);
	if (!charValue) {
		return false;
	}
	char* end;
	long value = strtol(charValue, &end, 10);
	if (*end) {
		return false;
	}
	*out = value;
	return true;
}

static bool _lookupUIntValue(const struct GBAConfig* config, const char* key, unsigned* out) {
	const char* charValue = _lookupValue(config, key);
	if (!charValue) {
		return false;
	}
	char* end;
	unsigned long value = strtoul(charValue, &end, 10);
	if (*end) {
		return false;
	}
	*out = value;
	return true;
}

static bool _lookupFloatValue(const struct GBAConfig* config, const char* key, float* out) {
	const char* charValue = _lookupValue(config, key);
	if (!charValue) {
		return false;
	}
	char* end;
	float value = strtof_u(charValue, &end);
	if (*end) {
		return false;
	}
	*out = value;
	return true;
}

void GBAConfigInit(struct GBAConfig* config, const char* port) {
	ConfigurationInit(&config->configTable);
	ConfigurationInit(&config->defaultsTable);
	if (port) {
		config->port = malloc(strlen("ports.") + strlen(port) + 1);
		snprintf(config->port, strlen("ports.") + strlen(port) + 1, "ports.%s", port);
	} else {
		config->port = 0;
	}
}

void GBAConfigDeinit(struct GBAConfig* config) {
	ConfigurationDeinit(&config->configTable);
	ConfigurationDeinit(&config->defaultsTable);
	free(config->port);
}

bool GBAConfigLoad(struct GBAConfig* config) {
	char path[PATH_MAX];
	GBAConfigDirectory(path, PATH_MAX);
	strncat(path, PATH_SEP "config.ini", PATH_MAX - strlen(path));
	return ConfigurationRead(&config->configTable, path);
}

bool GBAConfigSave(const struct GBAConfig* config) {
	char path[PATH_MAX];
	GBAConfigDirectory(path, PATH_MAX);
	strncat(path, PATH_SEP "config.ini", PATH_MAX - strlen(path));
	return ConfigurationWrite(&config->configTable, path);
}

void GBAConfigDirectory(char* out, size_t outLength) {
#ifndef _WIN32
	char* home = getenv("HOME");
	snprintf(out, outLength, "%s/.config", home);
	mkdir(out, 0755);
	snprintf(out, outLength, "%s/.config/%s", home, BINARY_NAME);
	mkdir(out, 0755);
#else
	char home[MAX_PATH];
	SHGetFolderPath(0, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, home);
	snprintf(out, outLength, "%s\\%s", home, "mGBA");
	CreateDirectoryA(out, NULL);
#endif
}

const char* GBAConfigGetValue(const struct GBAConfig* config, const char* key) {
	return _lookupValue(config, key);
}

void GBAConfigSetValue(struct GBAConfig* config, const char* key, const char* value) {
	ConfigurationSetValue(&config->configTable, config->port, key, value);
}

void GBAConfigSetIntValue(struct GBAConfig* config, const char* key, int value) {
	ConfigurationSetIntValue(&config->configTable, config->port, key, value);
}

void GBAConfigSetUIntValue(struct GBAConfig* config, const char* key, unsigned value) {
	ConfigurationSetUIntValue(&config->configTable, config->port, key, value);
}

void GBAConfigSetFloatValue(struct GBAConfig* config, const char* key, float value) {
	ConfigurationSetFloatValue(&config->configTable, config->port, key, value);
}

void GBAConfigSetDefaultValue(struct GBAConfig* config, const char* key, const char* value) {
	ConfigurationSetValue(&config->defaultsTable, config->port, key, value);
}

void GBAConfigSetDefaultIntValue(struct GBAConfig* config, const char* key, int value) {
	ConfigurationSetIntValue(&config->defaultsTable, config->port, key, value);
}

void GBAConfigSetDefaultUIntValue(struct GBAConfig* config, const char* key, unsigned value) {
	ConfigurationSetUIntValue(&config->defaultsTable, config->port, key, value);
}

void GBAConfigSetDefaultFloatValue(struct GBAConfig* config, const char* key, float value) {
	ConfigurationSetFloatValue(&config->defaultsTable, config->port, key, value);
}

void GBAConfigMap(const struct GBAConfig* config, struct GBAOptions* opts) {
	_lookupCharValue(config, "bios", &opts->bios);
	_lookupIntValue(config, "logLevel", &opts->logLevel);
	_lookupIntValue(config, "frameskip", &opts->frameskip);
	_lookupIntValue(config, "volume", &opts->volume);
	_lookupIntValue(config, "rewindBufferCapacity", &opts->rewindBufferCapacity);
	_lookupIntValue(config, "rewindBufferInterval", &opts->rewindBufferInterval);
	_lookupFloatValue(config, "fpsTarget", &opts->fpsTarget);
	unsigned audioBuffers;
	if (_lookupUIntValue(config, "audioBuffers", &audioBuffers)) {
		opts->audioBuffers = audioBuffers;
	}

	int fakeBool;
	if (_lookupIntValue(config, "useBios", &fakeBool)) {
		opts->useBios = fakeBool;
	}
	if (_lookupIntValue(config, "audioSync", &fakeBool)) {
		opts->audioSync = fakeBool;
	}
	if (_lookupIntValue(config, "videoSync", &fakeBool)) {
		opts->videoSync = fakeBool;
	}
	if (_lookupIntValue(config, "lockAspectRatio", &fakeBool)) {
		opts->lockAspectRatio = fakeBool;
	}
	if (_lookupIntValue(config, "resampleVideo", &fakeBool)) {
		opts->resampleVideo = fakeBool;
	}
	if (_lookupIntValue(config, "suspendScreensaver", &fakeBool)) {
		opts->suspendScreensaver = fakeBool;
	}
	if (_lookupIntValue(config, "mute", &fakeBool)) {
		opts->mute = fakeBool;
	}
	if (_lookupIntValue(config, "skipBios", &fakeBool)) {
		opts->skipBios = fakeBool;
	}
	if (_lookupIntValue(config, "rewindEnable", &fakeBool)) {
		opts->rewindEnable = fakeBool;
	}

	_lookupIntValue(config, "fullscreen", &opts->fullscreen);
	_lookupIntValue(config, "width", &opts->width);
	_lookupIntValue(config, "height", &opts->height);

	char* idleOptimization = 0;
	if (_lookupCharValue(config, "idleOptimization", &idleOptimization)) {
		if (strcasecmp(idleOptimization, "ignore") == 0) {
			opts->idleOptimization = IDLE_LOOP_IGNORE;
		} else if (strcasecmp(idleOptimization, "remove") == 0) {
			opts->idleOptimization = IDLE_LOOP_REMOVE;
		} else if (strcasecmp(idleOptimization, "detect") == 0) {
			opts->idleOptimization = IDLE_LOOP_DETECT;
		}
		free(idleOptimization);
	}
}

void GBAConfigLoadDefaults(struct GBAConfig* config, const struct GBAOptions* opts) {
	ConfigurationSetValue(&config->defaultsTable, 0, "bios", opts->bios);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "skipBios", opts->skipBios);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "useBios", opts->useBios);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "logLevel", opts->logLevel);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "frameskip", opts->frameskip);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "rewindEnable", opts->rewindEnable);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "rewindBufferCapacity", opts->rewindBufferCapacity);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "rewindBufferInterval", opts->rewindBufferInterval);
	ConfigurationSetFloatValue(&config->defaultsTable, 0, "fpsTarget", opts->fpsTarget);
	ConfigurationSetUIntValue(&config->defaultsTable, 0, "audioBuffers", opts->audioBuffers);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "audioSync", opts->audioSync);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "videoSync", opts->videoSync);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "fullscreen", opts->fullscreen);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "width", opts->width);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "height", opts->height);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "volume", opts->volume);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "mute", opts->mute);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "lockAspectRatio", opts->lockAspectRatio);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "resampleVideo", opts->resampleVideo);
	ConfigurationSetIntValue(&config->defaultsTable, 0, "suspendScreensaver", opts->suspendScreensaver);

	switch (opts->idleOptimization) {
	case IDLE_LOOP_IGNORE:
		ConfigurationSetValue(&config->defaultsTable, 0, "idleOptimization", "ignore");
		break;
	case IDLE_LOOP_REMOVE:
		ConfigurationSetValue(&config->defaultsTable, 0, "idleOptimization", "remove");
		break;
	case IDLE_LOOP_DETECT:
		ConfigurationSetValue(&config->defaultsTable, 0, "idleOptimization", "detect");
		break;
	}
}

// These two are basically placeholders in case the internal layout changes, e.g. for loading separate files
struct Configuration* GBAConfigGetInput(struct GBAConfig* config) {
	return &config->configTable;
}

struct Configuration* GBAConfigGetOverrides(struct GBAConfig* config) {
	return &config->configTable;
}

void GBAConfigFreeOpts(struct GBAOptions* opts) {
	free(opts->bios);
	opts->bios = 0;
}
