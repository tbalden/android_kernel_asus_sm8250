// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Vlad Adumitroaie <celtare21@gmail.com>.
 */

extern struct selinux_state *get_extern_state(void);
extern struct selinux_state *extern_state;
extern bool is_decrypted;
extern bool is_before_decryption;
