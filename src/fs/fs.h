/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#pragma once

void fsInit(void);

int fsRead(const char* name, void* data, unsigned size);
int fsWrite(const char* name, const void* data, unsigned size);
int fsRename(const char* oldName, const char* newName);
