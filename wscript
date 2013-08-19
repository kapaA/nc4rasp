#! /usr/bin/env python
# encoding: utf-8

import os

APPNAME = 'nc4rasp'
VERSION = '0.0.1'


def recurse_helper(ctx, name):
    if not ctx.has_dependency_path(name):
        ctx.fatal('Load a tool to find %s as system dependency' % name)
    else:
        p = ctx.dependency_path(name)
        ctx.recurse(p)

def options(opt):

    import waflib.extras.wurf_dependency_bundle as bundle
    import waflib.extras.wurf_dependency_resolve as resolve
    import waflib.extras.wurf_configure_output

    bundle.add_dependency(opt,
        resolve.ResolveGitMajorVersion(
            name = 'waf-tools',
            git_repository = 'github.com/steinwurf/external-waf-tools.git',
            major_version = 2))

    bundle.add_dependency(opt,
        resolve.ResolveGitMajorVersion(
            name = 'boost',
            git_repository = 'github.com/steinwurf/external-boost-light.git',
            major_version = 1))

    bundle.add_dependency(opt,
        resolve.ResolveGitMajorVersion(
            name = 'sak',
            git_repository = 'github.com/steinwurf/sak.git',
            major_version = 10))

    bundle.add_dependency(opt,
        resolve.ResolveGitMajorVersion(
            name = 'fifi',
            git_repository = 'github.com/steinwurf/fifi.git',
            major_version = 9))

    bundle.add_dependency(opt,
        resolve.ResolveGitMajorVersion(
            name = 'kodo',
            git_repository = 'github.com/steinwurf/kodo.git',
            major_version = 11))

    opt.load('wurf_dependency_bundle')
    opt.load('wurf_tools')


def configure(conf):

    if conf.is_toplevel():

        conf.load('wurf_dependency_bundle')
        conf.load('wurf_tools')

        conf.load_external_tool('mkspec', 'wurf_cxx_mkspec_tool')
        conf.load_external_tool('runners', 'wurf_runner')
        conf.load_external_tool('install_path', 'wurf_install_path')
        conf.load_external_tool('project_gen', 'wurf_project_generator')

        recurse_helper(conf, 'boost')
        recurse_helper(conf, 'sak')
        recurse_helper(conf, 'fifi')
        recurse_helper(conf, 'kodo')

def build(bld):

    if bld.is_toplevel():

        bld.load('wurf_dependency_bundle')

        recurse_helper(bld, 'boost')
        recurse_helper(bld, 'sak')
        recurse_helper(bld, 'fifi')
        recurse_helper(bld, 'kodo')

        # Only build test when executed from the
        # top-level wscript i.e. not when included as a dependency
        # in a recurse call
        bld.program(features = 'cxx',
              source   = bld.path.ant_glob('src/**/*.cpp'),
              target   = 'nc4rasp',
              use = ['kodo_includes', 'fifi_includes', 'sak_includes',
                     'boost_includes', 'boost_system', 'boost_thread',
                     'boost_chrono', 'boost_program_options'])
