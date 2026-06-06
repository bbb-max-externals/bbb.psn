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
      920.0,
      390.0
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
    "description": "Help for bbb.psn.sender external",
    "digest": "Send PosiStageNet tracker data",
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
            620.0,
            24.0
          ],
          "text": "bbb.psn.sender \u2014 send PSN tracker data over UDP"
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
            720.0,
            20.0
          ],
          "text": "Set tracker fields, then send data frames with bang/send. Send tracker names with info."
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
            180.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "name 1 performer"
        }
      },
      {
        "box": {
          "id": "obj-4",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            40.0,
            135.0,
            210.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "tracker 1 0. 1. 2. 0. 0. 0."
        }
      },
      {
        "box": {
          "id": "obj-5",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            40.0,
            165.0,
            110.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "info"
        }
      },
      {
        "box": {
          "id": "obj-6",
          "maxclass": "button",
          "numinlets": 1,
          "numoutlets": 1,
          "patching_rect": [
            165.0,
            165.0,
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
            200.0,
            168.0,
            160.0,
            20.0
          ],
          "text": "bang sends data"
        }
      },
      {
        "box": {
          "id": "obj-8",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "patching_rect": [
            40.0,
            220.0,
            390.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "bbb.psn.sender @destination 236.10.10.10 @port 56565 @system Max"
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
            280.0,
            95.0,
            22.0
          ],
          "text": "print psn-send"
        }
      },
      {
        "box": {
          "id": "obj-10",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            470.0,
            105.0,
            370.0,
            20.0
          ],
          "text": "Messages: pos, ori, speed, accel, target, status, name, clear"
        }
      },
      {
        "box": {
          "id": "obj-11",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            470.0,
            135.0,
            420.0,
            20.0
          ],
          "text": "tracker id x y z [yaw pitch roll] is the common compact form"
        }
      },
      {
        "box": {
          "id": "obj-12",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            470.0,
            165.0,
            385.0,
            20.0
          ],
          "text": "Outlet: sent <data|info> <packet-count>, error <message>"
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
            "obj-8",
            0
          ],
          "destination": [
            "obj-9",
            0
          ]
        }
      }
    ]
  }
}