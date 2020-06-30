# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from webports import package, configuration, util, error


def RemoveEmptyDirs(dirname):
  """Recursively remove a directoy and its parents if they are empty."""
  while not os.listdir(dirname):
    os.rmdir(dirname)
    dirname = os.path.dirname(dirname)


def RemoveFile(filename):
  os.remove(filename)
  RemoveEmptyDirs(os.path.dirname(filename))


class InstalledPackage(package.Package):
  extra_keys = package.EXTRA_KEYS

  def __init__(self, info_file):
    super(InstalledPackage, self).__init__(info_file)
    self.config = configuration.Configuration(self.BUILD_ARCH,
                                              self.BUILD_TOOLCHAIN,
                                              self.BUILD_CONFIG == 'debug')

  def Uninstall(self):
    self.LogStatus('Uninstalling')
    self.DoUninstall(force=False)

  def Files(self):
    """Yields the list of files currently installed by this package."""
    file_list = self.GetListFile()
    if not os.path.exists(file_list):
      return
    with open(self.GetListFile()) as f:
      for line in f:
        yield line.strip()

  def DoUninstall(self, force):
    with util.InstallLock(self.config):
      if not force:
        for pkg in InstalledPackageIterator(self.config):
          if self.NAME in pkg.DEPENDS:
            raise error.Error("Unable to uninstall '%s' (depended on by '%s')" %
                (self.NAME, pkg.NAME))
      RemoveFile(self.GetInstallStamp())

      root = util.GetInstallRoot(self.config)
      for filename in self.Files():
        fullname = os.path.join(root, filename)
        if not os.path.lexists(fullname):
          util.Warn('File not found while uninstalling: %s' % fullname)
          continue
        util.LogVerbose('uninstall: %s' % filename)
        RemoveFile(fullname)

      if os.path.exists(self.GetListFile()):
        RemoveFile(self.GetListFile())

  def ReverseDependencies(self):
    """Yields the set of packages that depend directly on this one"""
    for pkg in InstalledPackageIterator(self.config):
      if self.NAME in pkg.DEPENDS:
        yield pkg


def InstalledPackageIterator(config):
  stamp_root = util.GetInstallStampRoot(config)
  if not os.path.exists(stamp_root):
    return

  for filename in os.listdir(stamp_root):
    if os.path.splitext(filename)[1] != '.info':
      continue
    info_file = os.path.join(stamp_root, filename)
    if os.path.exists(info_file):
      yield InstalledPackage(info_file)


def CreateInstalledPackage(package_name, config=None):
  stamp_root = util.GetInstallStampRoot(config)
  info_file = os.path.join(stamp_root, package_name + '.info')
  if not os.path.exists(info_file):
    raise error.Error('package not installed: %s [%s]' % (package_name, config))
  return InstalledPackage(info_file)
