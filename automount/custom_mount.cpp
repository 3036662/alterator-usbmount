#include "custom_mount.hpp"
#include "utils.hpp"
#include <acl/libacl.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <exception>
#include <filesystem>
#include <grp.h>
#include <pwd.h>
#include <stdexcept>
#include <string>
#include <sys/acl.h>
#include <sys/stat.h>
#include <sys/types.h>

CustomMount::CustomMount(const std::shared_ptr<spdlog::logger> &logger) noexcept
    : logger_(logger) {}

bool CustomMount::Mount(std::shared_ptr<UsbUdevDevice> &ptr_device,
                        const UidGid &uid_gid) noexcept {
  uid_ = uid_gid.user_id;
  gid_ = uid_gid.group_id;
  CreateAclMountPoint();
  return true;
}

bool CustomMount::CreateAclMountPoint() noexcept {
  std::string mount_point = mount_root;
  // get user name
  std::optional<std::string> user_name;
  passwd *pwd = getpwuid(uid_.value_or(0));
  if (pwd->pw_name != NULL)
    user_name = pwd->pw_name;
  if (user_name.has_value())
    mount_point += user_name.value();
  else
    mount_point += std::to_string(uid_.value_or(0));
  mount_point += '_';
  // get group name
  std::optional<std::string> group_name;
  group *grp = getgrgid(gid_.value_or(0));
  if (grp->gr_name != NULL)
    group_name = grp->gr_name;
  if (group_name.has_value())
    mount_point += group_name.value();
  else
    mount_point += std::to_string(gid_.value_or(0));
  // create acl dir if no exists
  try {
    std::filesystem::create_directories(mount_point);
    if (chmod(mount_point.c_str(), 0750) != 0)
      throw std::runtime_error("Chmod 0750 failed");
    // process ACL
    SetAcl(mount_point);

  } catch (const std::exception &ex) {
    logger_->error("Cant create mount point {}", mount_point);
    logger_->error(ex.what());
    return false;
  }
  // create endpoint

  return true;
}

void CustomMount::SetAcl(const std::string &mount_point) {
  // read acl
  using namespace utils::acl;
  acl_t acl = acl_get_file(mount_point.c_str(), ACL_TYPE_ACCESS);
  if (acl == NULL)
    throw std::runtime_error("Cant read ACL for mountpoint");
  // delete all ACL_USER and ACL_GROUP
  DeleteACLUserGroupMask(acl);
  acl_entry_t entry;
  acl_permset_t permset;
  // add ACL_USER
  if (acl_create_entry(&acl, &entry) != 0)
    throw std::runtime_error("Can't create ACL entry");
  if (acl_get_permset(entry, &permset) != 0)
    throw std::runtime_error("acl_get_permset failed");
  if (acl_add_perm(permset, ACL_READ | ACL_EXECUTE) != 0)
    throw std::runtime_error("acl_add_perm failed");
  if (acl_set_tag_type(entry, ACL_USER) != 0)
    throw std::runtime_error("acl_set_tag_type failed");
  uid_t uid = uid_.value_or(0);
  if (acl_set_qualifier(entry, &uid) != 0)
    throw std::runtime_error("acl_set_qualifier failed");
  if (acl_set_permset(entry, permset) != 0)
    throw std::runtime_error("acl_set_permset failed");
  // Add ACL_GROUP
  if (acl_create_entry(&acl, &entry) != 0)
    throw std::runtime_error("Can't create ACL entry for group");
  if (acl_get_permset(entry, &permset) != 0)
    throw std::runtime_error("acl_get_permset failed");
  if (acl_add_perm(permset, ACL_READ | ACL_EXECUTE) != 0)
    throw std::runtime_error("acl_add_perm failed");
  if (acl_set_tag_type(entry, ACL_GROUP) != 0)
    throw std::runtime_error("acl_set_tag_type failed");
  gid_t gid = gid_.value_or(0);
  if (acl_set_qualifier(entry, &gid) != 0)
    throw std::runtime_error("acl_set_qualifier failed");
  if (acl_set_permset(entry, permset) != 0)
    throw std::runtime_error("acl_set_permset failed");
  // Add ACL_MASK
  if (acl_create_entry(&acl, &entry) != 0)
    throw std::runtime_error("Can't create ACL entry for mask");
  if (acl_get_permset(entry, &permset) != 0)
    throw std::runtime_error("acl_get_permset failed");
  if (acl_add_perm(permset, ACL_READ | ACL_EXECUTE) != 0)
    throw std::runtime_error("acl_add_perm failed");
  if (acl_set_tag_type(entry, ACL_MASK) != 0)
    throw std::runtime_error("acl_set_tag_type failed");
  if (acl_set_permset(entry, permset) != 0)
    throw std::runtime_error("acl_set_permset failed");
  logger_->info(ToString(acl));
  if (acl_valid(acl) != 0) {
    logger_->warn("Acl is invalid");
    int last = 0;
    logger_->warn(acl_error(acl_check(acl, &last)));
    logger_->warn("last entry = {}", last);
  }
  if (acl_set_file(mount_point.c_str(), ACL_TYPE_ACCESS, acl) != 0) {
    logger_->error(strerror(errno));
    throw std::runtime_error("acl_set_file failed");
  }
  // free ACL
  if (acl_free(acl) != 0)
    logger_->warn("acl_free failed");
  logger_->info("ACL for {} successfully set", mount_point);
}