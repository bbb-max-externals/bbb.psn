{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 8,
      "minor": 6,
      "revision": 4
    },
    "classnamespace": "box",
    "rect": [
      100.0,
      100.0,
      840.0,
      430.0
    ],
    "bglocked": 1,
    "openrect": [
      0.0,
      0.0,
      0.0,
      0.0
    ],
    "openinpresentation": 0,
    "default_fontsize": 12.0,
    "default_fontface": 0,
    "default_fontname": "Arial",
    "gridonopen": 2,
    "gridsize": [
      15.0,
      15.0
    ],
    "gridsnaponopen": 0,
    "objectsnaponopen": 1,
    "statusbarvisible": 2,
    "toolbarvisible": 2,
    "lefttoolbarpinned": 0,
    "toptoolbarpinned": 0,
    "righttoolbarpinned": 0,
    "bottomtoolbarpinned": 0,
    "toolbars_unpinned_last_save": 0,
    "tallnewobj": 0,
    "boxanimatetime": 200,
    "enablehscroll": 1,
    "enablevscroll": 1,
    "devicewidth": 0,
    "description": "Help for bbb.psn.receiver external",
    "digest": "Receive PosiStageNet tracker data",
    "tags": "posistagenet, psn, udp, tracking",
    "style": "",
    "subpatcher_template": "",
    "assistshowspatchername": 0,
    "boxes": [
      {
        "box": {
          "id": "obj-1",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            40.0,
            25.0,
            560.0,
            24.0
          ],
          "text": "PosiStageNet \u2014 receive PSN tracker data over UDP"
        }
      },
      {
        "box": {
          "id": "obj-2",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            40.0,
            55.0,
            680.0,
            20.0
          ],
          "text": "Default: port 56565, multicast 236.10.10.10. Use multicast none for unicast receiving."
        }
      },
      {
        "box": {
          "id": "obj-3",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            40.0,
            105.0,
            50.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "start"
        }
      },
      {
        "box": {
          "id": "obj-4",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            100.0,
            105.0,
            45.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "stop"
        }
      },
      {
        "box": {
          "id": "obj-5",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            155.0,
            105.0,
            58.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "restart"
        }
      },
      {
        "box": {
          "id": "obj-6",
          "maxclass": "button",
          "numinlets": 1,
          "numoutlets": 1,
          "patching_rect": [
            225.0,
            105.0,
            24.0,
            24.0
          ],
          "outlettype": [
            "bang"
          ]
        }
      },
      {
        "box": {
          "id": "obj-7",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            255.0,
            108.0,
            190.0,
            20.0
          ],
          "text": "bang reports status"
        }
      },
      {
        "box": {
          "id": "obj-8",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 2,
          "patching_rect": [
            40.0,
            165.0,
            260.0,
            22.0
          ],
          "outlettype": [
            "",
            ""
          ],
          "text": "bbb.psn.receiver @autostart 0"
        }
      },
      {
        "box": {
          "id": "obj-9",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            40.0,
            235.0,
            115.0,
            22.0
          ],
          "text": "print psn-trackers"
        }
      },
      {
        "box": {
          "id": "obj-10",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            205.0,
            235.0,
            90.0,
            22.0
          ],
          "text": "print psn-info"
        }
      },
      {
        "box": {
          "id": "obj-11",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            335.0,
            158.0,
            470.0,
            44.0
          ],
          "text": "out 1: tracker <id> <name> <x> <y> <z> <yaw> <pitch> <roll> <status> <timestamp>"
        }
      },
      {
        "box": {
          "id": "obj-12",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            335.0,
            205.0,
            420.0,
            40.0
          ],
          "text": "out 2: server <name>, name <id> <name>, status <0|1>, error <message>"
        }
      },
      {
        "box": {
          "id": "obj-13",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            40.0,
            300.0,
            650.0,
            20.0
          ],
          "text": "Attributes: @port <1-65535>, @multicast <IPv4 group|none>, @autostart <0|1>"
        }
      },
      {
        "box": {
          "id": "obj-14",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            40.0,
            340.0,
            120.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "port 56565"
        }
      },
      {
        "box": {
          "id": "obj-15",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            170.0,
            340.0,
            190.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "multicast none"
        }
      },
      {
        "box": {
          "id": "obj-16",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            370.0,
            340.0,
            105.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "autostart 0"
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": [
            "obj-3",
            0
          ],
          "destination": [
            "obj-8",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-4",
            0
          ],
          "destination": [
            "obj-8",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-5",
            0
          ],
          "destination": [
            "obj-8",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-6",
            0
          ],
          "destination": [
            "obj-8",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-14",
            0
          ],
          "destination": [
            "obj-8",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-15",
            0
          ],
          "destination": [
            "obj-8",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-16",
            0
          ],
          "destination": [
            "obj-8",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-8",
            0
          ],
          "destination": [
            "obj-9",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-8",
            1
          ],
          "destination": [
            "obj-10",
            0
          ]
        }
      }
    ]
  }
}