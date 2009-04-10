/* libguestfs - the guestfsd daemon
 * Copyright (C) 2009 Red Hat Inc. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../src/guestfs_protocol.h"
#include "daemon.h"
#include "actions.h"

int
do_rmdir (const char *path)
{
  int r;

  NEED_ROOT (-1);
  ABS_PATH (path, -1);

  CHROOT_IN;
  r = rmdir (path);
  CHROOT_OUT;

  if (r == -1) {
    reply_with_perror ("rmdir: %s", path);
    return -1;
  }

  return 0;
}

/* This implementation is quick and dirty, and allows people to try
 * to remove parts of the initramfs (eg. "rm -r /..") but if people
 * do stupid stuff, who are we to try to stop them?
 */
int
do_rm_rf (const char *path)
{
  int r, len;
  char *buf, *err;

  NEED_ROOT (-1);
  ABS_PATH (path, -1);

  if (strcmp (path, "/") == 0) {
    reply_with_error ("rm -rf: cannot remove root directory");
    return -1;
  }

  len = strlen (path) + 9;
  buf = malloc (len);
  if (buf == NULL) {
    reply_with_perror ("malloc");
    return -1;
  }

  snprintf (buf, len, "/sysroot%s", path);

  r = command (NULL, &err, "rm", "-rf", buf);
  free (buf);

  /* rm -rf is never supposed to fail.  I/O errors perhaps? */
  if (r == -1) {
    reply_with_error ("rm -rf: %s: %s", path, err);
    free (err);
    return -1;
  }

  free (err);

  return 0;
}

int
do_mkdir (const char *path)
{
  int r;

  NEED_ROOT (-1);
  ABS_PATH (path, -1);

  CHROOT_IN;
  r = mkdir (path, 0777);
  CHROOT_OUT;

  if (r == -1) {
    reply_with_perror ("mkdir: %s", path);
    return -1;
  }

  return 0;
}

static int
recursive_mkdir (const char *path)
{
  int loop = 0;
  int r;
  char *ppath, *p;

 again:
  r = mkdir (path, 0777);
  if (r == -1) {
    if (!loop && errno == ENOENT) {
      loop = 1;			/* Stops it looping forever. */

      /* If we're at the root, and we failed, just give up. */
      if (path[0] == '/' && path[1] == '\0') return -1;

      /* Try to make the parent directory first. */
      ppath = strdup (path);
      if (ppath == NULL) return -1;

      p = strrchr (ppath, '/');
      if (p) *p = '\0';

      r = recursive_mkdir (ppath);
      free (ppath);

      if (r == -1) return -1;

      goto again;
    } else	  /* Failed for some other reason, so return error. */
      return -1;
  }
  return 0;
}

int
do_mkdir_p (const char *path)
{
  int r;

  NEED_ROOT (-1);
  ABS_PATH (path, -1);

  CHROOT_IN;
  r = recursive_mkdir (path);
  CHROOT_OUT;

  if (r == -1) {
    reply_with_perror ("mkdir -p: %s", path);
    return -1;
  }

  return 0;
}
