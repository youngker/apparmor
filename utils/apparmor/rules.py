# ------------------------------------------------------------------
#
#    Copyright (C) 2014 Canonical Ltd.
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of version 2 of the GNU General Public
#    License published by the Free Software Foundation.
#
# ------------------------------------------------------------------

class DBUS_Rule(object):
    actions = set()
    busses = set()
    names = set()
    paths = set()
    interfaces = set()
    members = set()
    peer_names = set()
    peer_labels = set()

    audit = False
    deny = False

    def __init__(self, actions=[], busses=[], names=[], paths=[], interfaces=[],
                 members=[], peer_names=[], peer_labels=[]):
        self.actions = set(actions)
        self.busses = set(busses)
        self.names = set(names)
        self.paths = set(paths)
        self.interfaces = set(interfaces)
        self.members = set(members)
        self.peer_name = set(peer_names)
        self.peer_labels = set(peer_labels)

    def serialize(self):
        out = "%s%s%s" % ('audit ' if self.audit else '',
                          'deny '  if self.deny else '',
                          'dbus')
        if len(self.actions) > 0:
            if len(self.actions) == 1:
                out += ' %s' % self.actions[0]
            else:
                out += ' (%s)' % (', '.join(self.actions))
        out += ','
        return out

class Raw_DBUS_Rule(object):
    audit = False
    deny = False

    def __init__(self, rule):
        self.rule = rule

    def serialize(self):
        return "%s%s%s" % ('audit ' if self.audit else '',
                           'deny '  if self.deny else '',
                           self.rule)
