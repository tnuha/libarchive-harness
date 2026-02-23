#include <sys/types.h>

#include <sys/stat.h>

#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 32KB
#define WRITE_MEM_LIMIT 1 << 15

static int clonedOps(struct archive_entry *entry) {
  // manipulate an entry
  struct archive_entry *clonedEntry =
      archive_entry_new(); // dummy entry to manipulate

  // link: https://linux.die.net/man/3/archive_entry
  archive_entry_acl(entry);
  int allow_count =
      archive_entry_acl_count(entry, ARCHIVE_ENTRY_ACL_TYPE_ALLOW);
  int deny_count = archive_entry_acl_count(entry, ARCHIVE_ENTRY_ACL_TYPE_DENY);
  int audit_count =
      archive_entry_acl_count(entry, ARCHIVE_ENTRY_ACL_TYPE_AUDIT);
  int alarm_count =
      archive_entry_acl_count(entry, ARCHIVE_ENTRY_ACL_TYPE_ALARM);
  int reported_count = archive_entry_acl_count(
      entry, ARCHIVE_ENTRY_ACL_TYPE_ALLOW | ARCHIVE_ENTRY_ACL_TYPE_DENY |
                 ARCHIVE_ENTRY_ACL_TYPE_AUDIT | ARCHIVE_ENTRY_ACL_TYPE_ALARM);
  int summed_count = allow_count + deny_count + audit_count + alarm_count;
  if (summed_count != reported_count) {
    archive_entry_free(clonedEntry);
    return 1;
  }

  time_t atime = archive_entry_atime(entry);
  time_t atime_nsec = archive_entry_atime_nsec(entry);
  archive_entry_set_atime(clonedEntry, atime, atime_nsec);

  dev_t dev = archive_entry_dev(entry);
  archive_entry_set_dev(clonedEntry, dev);

  dev_t devmajor = archive_entry_devmajor(entry);
  archive_entry_set_devmajor(clonedEntry, devmajor);

  dev_t devminor = archive_entry_devminor(entry);
  archive_entry_set_devminor(clonedEntry, devminor);

  archive_entry_dev_is_set(entry);
  mode_t filetype = archive_entry_filetype(entry);
  archive_entry_set_filetype(clonedEntry, filetype);

  const char *hardlink = archive_entry_hardlink(entry);
  archive_entry_set_hardlink(clonedEntry, hardlink);
  archive_entry_copy_hardlink(clonedEntry, hardlink);

  la_int64_t ino = archive_entry_ino(entry);
  archive_entry_set_ino(clonedEntry, ino);

  mode_t mode = archive_entry_mode(entry);
  archive_entry_set_mode(clonedEntry, mode);

  time_t mtime = archive_entry_mtime(entry);
  time_t mtime_nsec = archive_entry_mtime_nsec(entry);
  archive_entry_set_mtime(clonedEntry, mtime, mtime_nsec);

  unsigned int nlink = archive_entry_nlink(entry);
  archive_entry_set_nlink(clonedEntry, nlink);

  const char *pathname = archive_entry_pathname(entry);
  archive_entry_set_pathname(clonedEntry, pathname);
  archive_entry_copy_pathname(clonedEntry, pathname);

  const wchar_t *pathname_w = archive_entry_pathname_w(entry);
  archive_entry_copy_pathname_w(clonedEntry, pathname_w);

  (void)archive_entry_rdev(entry);
  dev_t rdevmajor = archive_entry_rdevmajor(entry);
  dev_t rdevminor = archive_entry_rdevminor(entry);
  archive_entry_set_rdevmajor(clonedEntry, rdevmajor);
  archive_entry_set_rdevminor(clonedEntry, rdevminor);

  const char *gname = archive_entry_gname(entry);
  archive_entry_set_gname(clonedEntry, gname);
  archive_entry_copy_gname(clonedEntry, gname);

  const wchar_t *gname_w = archive_entry_gname_w(entry);
  archive_entry_copy_gname_w(clonedEntry, gname_w);

  long long gid = archive_entry_gid(entry);
  archive_entry_set_gid(clonedEntry, gid);

  long long uid = archive_entry_uid(entry);
  archive_entry_set_uid(clonedEntry, uid);

  int64_t s = archive_entry_size(entry);
  archive_entry_set_size(clonedEntry, s);

  const char *sourcepath = archive_entry_sourcepath(entry);
  archive_entry_copy_sourcepath(clonedEntry, sourcepath);

  const char *symlink = archive_entry_symlink(entry);
  archive_entry_set_symlink(clonedEntry, symlink);

  const struct stat *stat = archive_entry_stat(entry);
  archive_entry_copy_stat(clonedEntry, stat);

  const char *uname = archive_entry_uname(entry);
  archive_entry_set_uname(clonedEntry, uname);
  archive_entry_copy_uname(clonedEntry, uname);

  const wchar_t *uname_w = archive_entry_uname_w(entry);
  archive_entry_copy_uname_w(clonedEntry, uname_w);

  archive_entry_free(clonedEntry);

  // clone
  clonedEntry = archive_entry_clone(entry);

  archive_entry_acl_clear(clonedEntry);

  archive_entry_free(clonedEntry);

  return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  struct archive *a;
  struct archive_entry *entry;
  size_t out_used =
      0; // unclear if the `outUsed` pointer param is added to in-place, or if
         // it's assigned. Initializing to 0 is conservative.
  int r;

  char written[WRITE_MEM_LIMIT];

  a = archive_read_new();
  archive_read_support_format_tar(a);
  archive_read_support_format_cpio(a);
  archive_read_support_format_lha(a);
  archive_read_support_format_ar(a);
  archive_read_support_format_mtree(a);

  if (ARCHIVE_OK != archive_read_open_memory(a, data, size)) {
    archive_read_free(a);
    return 0;
  }

  for (;;) {
    r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF)
      break;
    if (r != ARCHIVE_OK) {
      archive_read_free(a);
      return 0;
    }

    if (clonedOps(entry)) {
      archive_read_free(a);
      return 1;
    }
  }

  // do a single write to the memory buffer
  // should be at most WRITE_MEM_LIMIT bytes used
  // referenced:
  // https://manpages.debian.org/jessie/libarchive-dev/archive_write_open.3.en.html
  // A convenience form of archive_write_open() that accepts a pointer to a
  // block of memory that will receive the archive. The final size_t * argument
  // points to a variable that will be updated after each write to reflect how
  // much of the buffer is currently in use. You should be careful to ensure
  // that this variable remains allocated until after the archive is closed.
  r = archive_write_open_memory(a, written, WRITE_MEM_LIMIT, &out_used);
  if (out_used > WRITE_MEM_LIMIT) {
    archive_read_free(a);
    return 1;
  }
  if (r != ARCHIVE_OK) {
    archive_read_free(a);
    return 0;
  }

  archive_read_close(a);
  archive_read_free(a);

  // no issues, huzzah
  return 0;
}
