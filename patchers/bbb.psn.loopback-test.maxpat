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
      960.0,
      540.0
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
    "description": "Local loopback test for bbb.psn sender/receiver",
    "digest": "Test bbb.psn sender and receiver locally",
    "tags": "posistagenet,psn,test,loopback",
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
            35.0,
            25.0,
            760.0,
            22.0
          ],
          "text": "bbb.psn local loopback test \u2014 send PSN packets to receiver on localhost"
        }
      },
      {
        "box": {
          "id": "obj-2",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            35.0,
            52.0,
            820.0,
            36.0
          ],
          "text": "Expected: click start, click name/info, click tracker, then bang/send. Receiver print should show server/name/tracker messages."
        }
      },
      {
        "box": {
          "id": "obj-3",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            35.0,
            115.0,
            45.0,
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
            90.0,
            115.0,
            42.0,
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
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 2,
          "patching_rect": [
            35.0,
            155.0,
            330.0,
            22.0
          ],
          "outlettype": [
            "",
            ""
          ],
          "text": "bbb.psn.receiver @port 56565 @multicast none @autostart 0"
        }
      },
      {
        "box": {
          "id": "obj-6",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            35.0,
            215.0,
            110.0,
            22.0
          ],
          "text": "print psn-rx-data"
        }
      },
      {
        "box": {
          "id": "obj-7",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            185.0,
            215.0,
            110.0,
            22.0
          ],
          "text": "print psn-rx-info"
        }
      },
      {
        "box": {
          "id": "obj-8",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            390.0,
            150.0,
            520.0,
            36.0
          ],
          "text": "Receiver uses @multicast none here, so it binds the port without joining a multicast group."
        }
      },
      {
        "box": {
          "id": "obj-9",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            35.0,
            310.0,
            155.0,
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
          "id": "obj-10",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            205.0,
            310.0,
            42.0,
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
          "id": "obj-11",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            35.0,
            345.0,
            245.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "tracker 1 1. 2. 3. 10. 20. 30."
        }
      },
      {
        "box": {
          "id": "obj-12",
          "maxclass": "button",
          "numinlets": 1,
          "numoutlets": 1,
          "patching_rect": [
            295.0,
            345.0,
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
          "id": "obj-13",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            330.0,
            348.0,
            130.0,
            20.0
          ],
          "text": "bang sends data"
        }
      },
      {
        "box": {
          "id": "obj-14",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "patching_rect": [
            35.0,
            400.0,
            380.0,
            22.0
          ],
          "outlettype": [
            ""
          ],
          "text": "bbb.psn.sender @destination 127.0.0.1 @port 56565 @system LocalMax"
        }
      },
      {
        "box": {
          "id": "obj-15",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            35.0,
            455.0,
            110.0,
            22.0
          ],
          "text": "print psn-tx"
        }
      },
      {
        "box": {
          "id": "obj-16",
          "maxclass": "comment",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            455.0,
            398.0,
            480.0,
            38.0
          ],
          "text": "This patch uses unicast loopback. Use Wireshark display filter udp.port == 56565 if you need packet-level proof."
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
            "obj-5",
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
            "obj-5",
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
            "obj-6",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-5",
            1
          ],
          "destination": [
            "obj-7",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-9",
            0
          ],
          "destination": [
            "obj-14",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-10",
            0
          ],
          "destination": [
            "obj-14",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-11",
            0
          ],
          "destination": [
            "obj-14",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-12",
            0
          ],
          "destination": [
            "obj-14",
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
            "obj-15",
            0
          ]
        }
      }
    ]
  }
}