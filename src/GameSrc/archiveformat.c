/*

Copyright (C) 2020 Shockolate Project

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "archiveformat.h"
#include "effect.h"
#include "lvldata.h"
#include "map.h"
#include "objcrit.h"
#include "objects.h"
#include "objgame.h"
#include "objstuff.h"
#include "objwpn.h"
#include "objwarez.h"
#include "pathfind.h"
#include "schedtyp.h"
#include "schedule.h"
#include "textmaps.h"
#include "trigger.h"

#define RES_FORMAT(layout) \
    { ResDecode, ResEncode, (UserDecodeData)&layout, NULL }

const ResLayout U32Layout = {
    4, sizeof(uint32_t), 0, {
	{ RFFT_UINT32, 0 },
	{ RFFT_END,    0 }
    }
};
const ResourceFormat U32Format = RES_FORMAT(U32Layout);

const ResLayout U16Layout = {
    2, sizeof(uint16_t), 0, {
	{ RFFT_UINT16, 0 },
	{ RFFT_END,    0 }
    }
};

// Schedule layout.
const ResLayout ScheduleLayout = {
    22, sizeof(Schedule), 0, {
	{ RFFT_UINT32, offsetof(Schedule, queue.size)     },
	{ RFFT_UINT32, offsetof(Schedule, queue.fullness) },
	{ RFFT_UINT32, offsetof(Schedule, queue.elemsize) },
	{ RFFT_UINT8,  offsetof(Schedule, queue.grow)     },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_PAD,    4 }, // pointer member
	{ RFFT_PAD,    4 }, // pointer member
	{ RFFT_END,    0 }
    }
};
const ResourceFormat ScheduleFormat = RES_FORMAT(ScheduleLayout);

// Schedule queue element. This has several possible formats which I'm treating
// as plain binary for the time being.
const ResLayout ScheduleQueueLayout = {
    8, sizeof(SchedEvent), LAYOUT_FLAG_ARRAY, {
	{ RFFT_UINT16, offsetof(SchedEvent, timestamp) },
	{ RFFT_UINT16, offsetof(SchedEvent, type)      },
	{ RFFT_BIN(SCHED_DATASIZ), offsetof(SchedEvent, data) },
	{ RFFT_END,    0 }
    }
};
const ResourceFormat ScheduleQueueFormat = RES_FORMAT(ScheduleQueueLayout);

// Describe the layout of the map info structure (FullMap). While technically
// this ends in an array, in practice it only ever has a single entry so just
// treat it as a flat structure.
const ResLayout FullMapLayout = {
    58,              // size on disc
    sizeof(FullMap), // size in memory
    0,               // flags
    {
	{ RFFT_UINT32, offsetof(FullMap, x_size)                  },
	{ RFFT_UINT32, offsetof(FullMap, y_size)                  },
	{ RFFT_UINT32, offsetof(FullMap, x_shft)                  },
	{ RFFT_UINT32, offsetof(FullMap, y_shft)                  },
	{ RFFT_UINT32, offsetof(FullMap, z_shft)                  },
	{ RFFT_PAD,    4 /* map elems pointer */                  },
	{ RFFT_UINT8,  offsetof(FullMap, cyber)                   },
	{ RFFT_UINT32, offsetof(FullMap, x_scale)                 },
	{ RFFT_UINT32, offsetof(FullMap, y_scale)                 },
	{ RFFT_UINT32, offsetof(FullMap, z_scale)                 },
	{ RFFT_UINT32, offsetof(FullMap, sched[0].queue.size)     },
	{ RFFT_UINT32, offsetof(FullMap, sched[0].queue.fullness) },
	{ RFFT_UINT32, offsetof(FullMap, sched[0].queue.elemsize) },
	{ RFFT_UINT8,  offsetof(FullMap, sched[0].queue.grow)     },
	{ RFFT_PAD,    12 /* 3 pointers at end */                 },
	{ RFFT_END,    0 }
    }
};
const ResourceFormat FullMapFormat = RES_FORMAT(FullMapLayout);

// Describe the layout of a map element (tile; MapElem).
const ResLayout MapElemLayout = {
    16,                // size on disc
    sizeof(MapElem),   // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT8,  offsetof(MapElem, tiletype)     },
	{ RFFT_UINT8,  offsetof(MapElem, flr_rotnhgt)  },
	{ RFFT_UINT8,  offsetof(MapElem, ceil_rotnhgt) },
	{ RFFT_UINT8,  offsetof(MapElem, param)        },
	{ RFFT_UINT16, offsetof(MapElem, objRef)       },
	{ RFFT_UINT16, offsetof(MapElem, tmap_ccolor)  },
	{ RFFT_UINT8,  offsetof(MapElem, flag1)        },
	{ RFFT_UINT8,  offsetof(MapElem, flag2)        },
	{ RFFT_UINT8,  offsetof(MapElem, flag3)        },
	{ RFFT_UINT8,  offsetof(MapElem, flag4)        },
	{ RFFT_UINT8,  offsetof(MapElem, sub_clip)     },
	{ RFFT_UINT8,  offsetof(MapElem, clearsolid)   },
	{ RFFT_UINT8,  offsetof(MapElem, flick_qclip)  },
	{ RFFT_UINT8,  offsetof(MapElem, templight)    },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of level texture info (array of 16-bit texture IDs).
const ResLayout TextureInfoLayout = {
    2,                 // size on disc
    sizeof(short),     // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, 0 },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the objects table in a resfile. (27-byte PC record).
const ResLayout ObjV11Layout = {
    27,                // size on disc
    sizeof(Obj),       // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT8,  offsetof(Obj, active)              },
	{ RFFT_UINT8,  offsetof(Obj, obclass)             },
	{ RFFT_UINT8,  offsetof(Obj, subclass)            },
	{ RFFT_UINT16, offsetof(Obj, specID)              },
	{ RFFT_UINT16, offsetof(Obj, ref)                 },
	{ RFFT_UINT16, offsetof(Obj, next)                },
	{ RFFT_UINT16, offsetof(Obj, prev)                },
	{ RFFT_UINT16, offsetof(Obj, loc.x)               },
	{ RFFT_UINT16, offsetof(Obj, loc.y)               },
	{ RFFT_UINT8,  offsetof(Obj, loc.z)               },
	{ RFFT_UINT8,  offsetof(Obj, loc.p)               },
	{ RFFT_UINT8,  offsetof(Obj, loc.h)               },
	{ RFFT_UINT8,  offsetof(Obj, loc.b)               },
	{ RFFT_UINT8,  offsetof(Obj, info.ph)             },
	{ RFFT_UINT8,  offsetof(Obj, info.type)           },
	{ RFFT_UINT16, offsetof(Obj, info.current_hp)     },
	{ RFFT_UINT8,  offsetof(Obj, info.make_info)      },
	{ RFFT_UINT8,  offsetof(Obj, info.current_frame)  },
	{ RFFT_UINT8,  offsetof(Obj, info.time_remainder) },
	{ RFFT_UINT8,  offsetof(Obj, info.inst_flags)     },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the objects table in a resfile. ("Easysaves" v12
// record; has an extra byte of padding due to 16-bit alignment on Mac).
const ResLayout ObjV12Layout = {
    28,                // size on disc
    sizeof(Obj),       // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT8,  offsetof(Obj, active)              },
	{ RFFT_UINT8,  offsetof(Obj, obclass)             },
	{ RFFT_UINT8,  offsetof(Obj, subclass)            },
	{ RFFT_PAD,    1 }, // 2-byte alignment
	{ RFFT_UINT16, offsetof(Obj, specID)              },
	{ RFFT_UINT16, offsetof(Obj, ref)                 },
	{ RFFT_UINT16, offsetof(Obj, next)                },
	{ RFFT_UINT16, offsetof(Obj, prev)                },
	{ RFFT_UINT16, offsetof(Obj, loc.x)               },
	{ RFFT_UINT16, offsetof(Obj, loc.y)               },
	{ RFFT_UINT8,  offsetof(Obj, loc.z)               },
	{ RFFT_UINT8,  offsetof(Obj, loc.p)               },
	{ RFFT_UINT8,  offsetof(Obj, loc.h)               },
	{ RFFT_UINT8,  offsetof(Obj, loc.b)               },
	{ RFFT_UINT8,  offsetof(Obj, info.ph)             },
	{ RFFT_UINT8,  offsetof(Obj, info.type)           },
	{ RFFT_UINT16, offsetof(Obj, info.current_hp)     },
	{ RFFT_UINT8,  offsetof(Obj, info.make_info)      },
	{ RFFT_UINT8,  offsetof(Obj, info.current_frame)  },
	{ RFFT_UINT8,  offsetof(Obj, info.time_remainder) },
	{ RFFT_UINT8,  offsetof(Obj, info.inst_flags)     },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the object cross-refs table in a resfile.
const ResLayout ObjRefLayout = {
    10,                // size on disc
    sizeof(ObjRef),    // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, offsetof(ObjRef, state.bin.sq.x) },
	{ RFFT_UINT16, offsetof(ObjRef, state.bin.sq.y) },
	{ RFFT_UINT16, offsetof(ObjRef, obj)            },
	{ RFFT_UINT16, offsetof(ObjRef, next)           },
	{ RFFT_UINT16, offsetof(ObjRef, nextref)        },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the gun class info in a resfile.
const ResLayout GunLayout = {
    8,                 // size on disc
    sizeof(ObjGun),    // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, offsetof(ObjGun, id)         },
	{ RFFT_UINT16, offsetof(ObjGun, next)       },
	{ RFFT_UINT16, offsetof(ObjGun, prev)       },
	{ RFFT_UINT8,  offsetof(ObjGun, ammo_type)  },
	{ RFFT_UINT8,  offsetof(ObjGun, ammo_count) },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the ammo class info in a resfile.
const ResLayout AmmoLayout = {
    6,                 // size on disc
    sizeof(ObjAmmo),   // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, offsetof(ObjAmmo, id)   },
	{ RFFT_UINT16, offsetof(ObjAmmo, next) },
	{ RFFT_UINT16, offsetof(ObjAmmo, prev) },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the physics object class info in a resfile.
const ResLayout PhysicsLayout = {
    40,                 // size on disc
    sizeof(ObjPhysics), // size in memory
    LAYOUT_FLAG_ARRAY,  // flags
    {
	{ RFFT_UINT16, offsetof(ObjPhysics, id)            },
	{ RFFT_UINT16, offsetof(ObjPhysics, next)          },
	{ RFFT_UINT16, offsetof(ObjPhysics, prev)          },
	{ RFFT_UINT16, offsetof(ObjPhysics, owner)         },
	{ RFFT_UINT32, offsetof(ObjPhysics, bullet_triple) },
	{ RFFT_UINT32, offsetof(ObjPhysics, duration)      },
	{ RFFT_UINT16, offsetof(ObjPhysics, p1.x)          },
	{ RFFT_UINT16, offsetof(ObjPhysics, p1.y)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p1.z)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p1.p)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p1.h)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p1.b)          },
	{ RFFT_UINT16, offsetof(ObjPhysics, p2.x)          },
	{ RFFT_UINT16, offsetof(ObjPhysics, p2.y)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p2.z)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p2.p)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p2.h)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p2.b)          },
	{ RFFT_UINT16, offsetof(ObjPhysics, p3.x)          },
	{ RFFT_UINT16, offsetof(ObjPhysics, p3.y)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p3.z)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p3.p)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p3.h)          },
	{ RFFT_UINT8,  offsetof(ObjPhysics, p3.b)          },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the grenade class info in a resfile.
const ResLayout GrenadeLayout = {
    12,                 // size on disc
    sizeof(ObjGrenade), // size in memory
    LAYOUT_FLAG_ARRAY,  // flags
    {
	{ RFFT_UINT16, offsetof(ObjGrenade, id)            },
	{ RFFT_UINT16, offsetof(ObjGrenade, next)          },
	{ RFFT_UINT16, offsetof(ObjGrenade, prev)          },
	{ RFFT_UINT8,  offsetof(ObjGrenade, unique_id)     },
	{ RFFT_UINT8,  offsetof(ObjGrenade, walls_hit)     },
	{ RFFT_UINT16, offsetof(ObjGrenade, flags)         },
	{ RFFT_UINT16, offsetof(ObjGrenade, timestamp)     },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the drug class info in a resfile.
const ResLayout DrugLayout = {
    6,                 // size on disc
    sizeof(ObjDrug),   // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, offsetof(ObjDrug, id)   },
	{ RFFT_UINT16, offsetof(ObjDrug, next) },
	{ RFFT_UINT16, offsetof(ObjDrug, prev) },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the hardware class info in a resfile (original 7-byte
// v11 struct).
const ResLayout HardwareV11Layout = {
    7,                   // size on disc
    sizeof(ObjHardware), // size in memory
    LAYOUT_FLAG_ARRAY,   // flags
    {
	{ RFFT_UINT16, offsetof(ObjHardware, id)      },
	{ RFFT_UINT16, offsetof(ObjHardware, next)    },
	{ RFFT_UINT16, offsetof(ObjHardware, prev)    },
	{ RFFT_UINT8,  offsetof(ObjHardware, version) },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the hardware class info in a resfile ("easysaves"
// 8-byte v12 struct).
const ResLayout HardwareV12Layout = {
    8,                   // size on disc
    sizeof(ObjHardware), // size in memory
    LAYOUT_FLAG_ARRAY,   // flags
    {
	{ RFFT_UINT16, offsetof(ObjHardware, id)      },
	{ RFFT_UINT16, offsetof(ObjHardware, next)    },
	{ RFFT_UINT16, offsetof(ObjHardware, prev)    },
	{ RFFT_UINT8,  offsetof(ObjHardware, version) },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the software class info in a resfile (original 9-byte
// v11 struct).
const ResLayout SoftwareV11Layout = {
    9,                   // size on disc
    sizeof(ObjSoftware), // size in memory
    LAYOUT_FLAG_ARRAY,   // flags
    {
	{ RFFT_UINT16, offsetof(ObjSoftware, id)         },
	{ RFFT_UINT16, offsetof(ObjSoftware, next)       },
	{ RFFT_UINT16, offsetof(ObjSoftware, prev)       },
	{ RFFT_UINT8,  offsetof(ObjSoftware, version)    },
	{ RFFT_UINT16, offsetof(ObjSoftware, data_munge) },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the software class info in a resfile (10-byte v12
// struct).
const ResLayout SoftwareV12Layout = {
    10,                  // size on disc
    sizeof(ObjSoftware), // size in memory
    LAYOUT_FLAG_ARRAY,   // flags
    {
	{ RFFT_UINT16, offsetof(ObjSoftware, id)         },
	{ RFFT_UINT16, offsetof(ObjSoftware, next)       },
	{ RFFT_UINT16, offsetof(ObjSoftware, prev)       },
	{ RFFT_UINT8,  offsetof(ObjSoftware, version)    },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_UINT16, offsetof(ObjSoftware, data_munge) },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the "bigstuff" class info in a resfile.
const ResLayout BigStuffLayout = {
    16,                  // size on disc
    sizeof(ObjBigstuff), // size in memory
    LAYOUT_FLAG_ARRAY,   // flags
    {
	{ RFFT_UINT16, offsetof(ObjBigstuff, id)             },
	{ RFFT_UINT16, offsetof(ObjBigstuff, next)           },
	{ RFFT_UINT16, offsetof(ObjBigstuff, prev)           },
	{ RFFT_UINT16, offsetof(ObjBigstuff, cosmetic_value) },
	{ RFFT_UINT32, offsetof(ObjBigstuff, data1)          },
	{ RFFT_UINT32, offsetof(ObjBigstuff, data2)          },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the "smallstuff" class info in a resfile.
const ResLayout SmallStuffLayout = {
    16,                    // size on disc
    sizeof(ObjSmallstuff), // size in memory
    LAYOUT_FLAG_ARRAY,     // flags
    {
	{ RFFT_UINT16, offsetof(ObjSmallstuff, id)             },
	{ RFFT_UINT16, offsetof(ObjSmallstuff, next)           },
	{ RFFT_UINT16, offsetof(ObjSmallstuff, prev)           },
	{ RFFT_UINT16, offsetof(ObjSmallstuff, cosmetic_value) },
	{ RFFT_UINT32, offsetof(ObjSmallstuff, data1)          },
	{ RFFT_UINT32, offsetof(ObjSmallstuff, data2)          },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the fixture class info in a resfile.
const ResLayout FixtureLayout = {
    30,                 // size on disc
    sizeof(ObjFixture), // size in memory
    LAYOUT_FLAG_ARRAY,  // flags
    {
	{ RFFT_UINT16, offsetof(ObjFixture, id)             },
	{ RFFT_UINT16, offsetof(ObjFixture, next)           },
	{ RFFT_UINT16, offsetof(ObjFixture, prev)           },
	{ RFFT_UINT8,  offsetof(ObjFixture, trap_type)      },
	{ RFFT_UINT8,  offsetof(ObjFixture, destroy_count)  },
	{ RFFT_UINT32, offsetof(ObjFixture, comparator)     },
	{ RFFT_UINT32, offsetof(ObjFixture, p1)             },
	{ RFFT_UINT32, offsetof(ObjFixture, p2)             },
	{ RFFT_UINT32, offsetof(ObjFixture, p3)             },
	{ RFFT_UINT32, offsetof(ObjFixture, p4)             },
	{ RFFT_UINT16, offsetof(ObjFixture, access_level)   },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the door class info in a resfile.
const ResLayout DoorLayout = {
    14,                // size on disc
    sizeof(ObjDoor),   // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, offsetof(ObjDoor, id)             },
	{ RFFT_UINT16, offsetof(ObjDoor, next)           },
	{ RFFT_UINT16, offsetof(ObjDoor, prev)           },
	{ RFFT_UINT16, offsetof(ObjDoor, locked)         },
	{ RFFT_UINT8,  offsetof(ObjDoor, stringnum)      },
	{ RFFT_UINT8,  offsetof(ObjDoor, cosmetic_value) },
	{ RFFT_UINT8,  offsetof(ObjDoor, access_level)   },
	{ RFFT_UINT8,  offsetof(ObjDoor, autoclose_time) },
	{ RFFT_UINT16, offsetof(ObjDoor, other_half)     },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the animating class info in a resfile.
const ResLayout AnimatingLayout = {
    10,                   // size on disc
    sizeof(ObjAnimating), // size in memory
    LAYOUT_FLAG_ARRAY,    // flags
    {
	{ RFFT_UINT16, offsetof(ObjAnimating, id)             },
	{ RFFT_UINT16, offsetof(ObjAnimating, next)           },
	{ RFFT_UINT16, offsetof(ObjAnimating, prev)           },
	{ RFFT_UINT8,  offsetof(ObjAnimating, start_frame)    },
	{ RFFT_UINT8,  offsetof(ObjAnimating, end_frame)      },
	{ RFFT_UINT16, offsetof(ObjAnimating, owner)          },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the trap class info in a resfile.
const ResLayout TrapLayout = {
    28,                // size on disc
    sizeof(ObjTrap),   // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, offsetof(ObjTrap, id)             },
	{ RFFT_UINT16, offsetof(ObjTrap, next)           },
	{ RFFT_UINT16, offsetof(ObjTrap, prev)           },
	{ RFFT_UINT8,  offsetof(ObjTrap, trap_type)      },
	{ RFFT_UINT8,  offsetof(ObjTrap, destroy_count)  },
	{ RFFT_UINT32, offsetof(ObjTrap, comparator)     },
	{ RFFT_UINT32, offsetof(ObjTrap, p1)             },
	{ RFFT_UINT32, offsetof(ObjTrap, p2)             },
	{ RFFT_UINT32, offsetof(ObjTrap, p3)             },
	{ RFFT_UINT32, offsetof(ObjTrap, p4)             },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the container class info in a resfile (original 21-
// byte struct).
const ResLayout ContainerV11Layout = {
    21,                   // size on disc
    sizeof(ObjContainer), // size in memory
    LAYOUT_FLAG_ARRAY,    // flags
    {
	{ RFFT_UINT16, offsetof(ObjContainer, id)          },
	{ RFFT_UINT16, offsetof(ObjContainer, next)        },
	{ RFFT_UINT16, offsetof(ObjContainer, prev)        },
	{ RFFT_UINT32, offsetof(ObjContainer, contents1)   },
	{ RFFT_UINT32, offsetof(ObjContainer, contents2)   },
	{ RFFT_UINT8,  offsetof(ObjContainer, dim_x)       },
	{ RFFT_UINT8,  offsetof(ObjContainer, dim_y)       },
	{ RFFT_UINT8,  offsetof(ObjContainer, dim_z)       },
	{ RFFT_UINT32, offsetof(ObjContainer, data1)       },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the container class info in a resfile (22-byte v12
// struct).
const ResLayout ContainerV12Layout = {
    22,                   // size on disc
    sizeof(ObjContainer), // size in memory
    LAYOUT_FLAG_ARRAY,    // flags
    {
	{ RFFT_UINT16, offsetof(ObjContainer, id)          },
	{ RFFT_UINT16, offsetof(ObjContainer, next)        },
	{ RFFT_UINT16, offsetof(ObjContainer, prev)        },
	{ RFFT_UINT32, offsetof(ObjContainer, contents1)   },
	{ RFFT_UINT32, offsetof(ObjContainer, contents2)   },
	{ RFFT_UINT8,  offsetof(ObjContainer, dim_x)       },
	{ RFFT_UINT8,  offsetof(ObjContainer, dim_y)       },
	{ RFFT_UINT8,  offsetof(ObjContainer, dim_z)       },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_UINT32, offsetof(ObjContainer, data1)       },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the critter class info in a resfile.
const ResLayout CritterLayout = {
    46,                 // size on disc
    sizeof(ObjCritter), // size in memory
    LAYOUT_FLAG_ARRAY,  // flags
    {
	{ RFFT_UINT16, offsetof(ObjCritter, id)              },
	{ RFFT_UINT16, offsetof(ObjCritter, next)            },
	{ RFFT_UINT16, offsetof(ObjCritter, prev)            },
	{ RFFT_UINT32, offsetof(ObjCritter, des_heading)     },
	{ RFFT_UINT32, offsetof(ObjCritter, des_speed)       },
	{ RFFT_UINT32, offsetof(ObjCritter, urgency)         },
	{ RFFT_UINT16, offsetof(ObjCritter, wait_frames)     },
	{ RFFT_UINT16, offsetof(ObjCritter, flags)           },
	{ RFFT_UINT32, offsetof(ObjCritter, attack_count)    },
	{ RFFT_UINT8,  offsetof(ObjCritter, ai_mode)         },
	{ RFFT_UINT8,  offsetof(ObjCritter, mood)            },
	{ RFFT_UINT8,  offsetof(ObjCritter, orders)          },
	{ RFFT_UINT8,  offsetof(ObjCritter, current_posture) },
	{ RFFT_UINT8,  offsetof(ObjCritter, x1)              },
	{ RFFT_UINT8,  offsetof(ObjCritter, y1)              },
	{ RFFT_UINT8,  offsetof(ObjCritter, dest_x)          },
	{ RFFT_UINT8,  offsetof(ObjCritter, dest_y)          },
	{ RFFT_UINT8,  offsetof(ObjCritter, pf_x)            },
	{ RFFT_UINT8,  offsetof(ObjCritter, pf_y)            },
	{ RFFT_UINT8,  offsetof(ObjCritter, path_id)         },
	{ RFFT_UINT8,  offsetof(ObjCritter, path_tries)      },
	{ RFFT_UINT16, offsetof(ObjCritter, loot1)           },
	{ RFFT_UINT16, offsetof(ObjCritter, loot2)           },
	{ RFFT_UINT32, offsetof(ObjCritter, sidestep)        },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of animation textures in a resfile (7-byte v11 struct).
const ResLayout AnimTextureV11Layout = {
    7,                       // size on disc
    sizeof(AnimTextureData), // size in memory
    LAYOUT_FLAG_ARRAY,       // flags
    {
	{ RFFT_UINT16, offsetof(AnimTextureData, anim_speed)     },
	{ RFFT_UINT16, offsetof(AnimTextureData, time_remainder) },
	{ RFFT_UINT8,  offsetof(AnimTextureData, current_frame)  },
	{ RFFT_UINT8,  offsetof(AnimTextureData, num_frames)     },
	{ RFFT_UINT8,  offsetof(AnimTextureData, flags)          },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of animation textures in a resfile (8-byte v12 struct).
const ResLayout AnimTextureV12Layout = {
    8,                       // size on disc
    sizeof(AnimTextureData), // size in memory
    LAYOUT_FLAG_ARRAY,       // flags
    {
	{ RFFT_UINT16, offsetof(AnimTextureData, anim_speed)     },
	{ RFFT_UINT16, offsetof(AnimTextureData, time_remainder) },
	{ RFFT_UINT8,  offsetof(AnimTextureData, current_frame)  },
	{ RFFT_UINT8,  offsetof(AnimTextureData, num_frames)     },
	{ RFFT_UINT8,  offsetof(AnimTextureData, flags)          },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_END,    0 }
    }
};

// Describes the layout of the hack cameras / hack surrogates tables. (Each is
// just an array of 16-bit ObjIDs.)
const ResLayout HackCameraLayout = {
    2,                 // size on disc
    sizeof(ObjID),     // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, 0 },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the level data in a resfile.
// FIXME explicitly copies the 3 automap info structs. Should support sub-
// arrays somehow in the layout table.
const ResLayout LevelDataV11Layout = {
    94,                // size on disc
    sizeof(LevelData), // size in memory
    0,                 // flags
    {
	{ RFFT_UINT16, offsetof(LevelData, size)                       },
	{ RFFT_UINT8,  offsetof(LevelData, mist)                       },
	{ RFFT_UINT8,  offsetof(LevelData, gravity)                    },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.rad)                 },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.bio)                 },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.zerogbio)            },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.bio_h)               },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.rad_h)               },
	{ RFFT_UINT32, offsetof(LevelData, exit_time)                  },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[0].init)          },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[0].zoom)          },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[0].xf)            },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[0].yf)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].lw)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].lh)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].obj_to_follow) },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].sensor_obj)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].note_obj)      },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].flags)         },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].avail_flags)   },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[0].version_id)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].sensor_rad)    },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[1].init)          },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[1].zoom)          },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[1].xf)            },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[1].yf)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].lw)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].lh)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].obj_to_follow) },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].sensor_obj)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].note_obj)      },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].flags)         },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].avail_flags)   },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[1].version_id)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].sensor_rad)    },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[2].init)          },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[2].zoom)          },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[2].xf)            },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[2].yf)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].lw)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].lh)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].obj_to_follow) },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].sensor_obj)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].note_obj)      },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].flags)         },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].avail_flags)   },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[2].version_id)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].sensor_rad)    },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the level data in a resfile (v12 data structure).
// FIXME explicitly copies the 3 automap info structs. Should support sub-
// arrays somehow in the layout table.
const ResLayout LevelDataV12Layout = {
    98,                // size on disc
    sizeof(LevelData), // size in memory
    0,                 // flags
    {
	{ RFFT_UINT16, offsetof(LevelData, size)                       },
	{ RFFT_UINT8,  offsetof(LevelData, mist)                       },
	{ RFFT_UINT8,  offsetof(LevelData, gravity)                    },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.rad)                 },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.bio)                 },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.zerogbio)            },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.bio_h)               },
	{ RFFT_UINT8,  offsetof(LevelData, hazard.rad_h)               },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_UINT32, offsetof(LevelData, exit_time)                  },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[0].init)          },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[0].zoom)          },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[0].xf)            },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[0].yf)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].lw)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].lh)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].obj_to_follow) },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].sensor_obj)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].note_obj)      },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].flags)         },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].avail_flags)   },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[0].version_id)    },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[0].sensor_rad)    },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[1].init)          },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[1].zoom)          },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[1].xf)            },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[1].yf)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].lw)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].lh)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].obj_to_follow) },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].sensor_obj)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].note_obj)      },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].flags)         },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].avail_flags)   },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[1].version_id)    },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[1].sensor_rad)    },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[2].init)          },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[2].zoom)          },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[2].xf)            },
	{ RFFT_UINT32, offsetof(LevelData, auto_maps[2].yf)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].lw)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].lh)            },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].obj_to_follow) },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].sensor_obj)    },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].note_obj)      },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].flags)         },
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].avail_flags)   },
	{ RFFT_UINT8,  offsetof(LevelData, auto_maps[2].version_id)    },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_UINT16, offsetof(LevelData, auto_maps[2].sensor_rad)    },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of a path.
const ResLayout PathLayout = {
    28,                // size on disc
    sizeof(Path),      // size in memory
    LAYOUT_FLAG_ARRAY, // flags
    {
	{ RFFT_UINT16, offsetof(Path, source.x)                       },
	{ RFFT_UINT16, offsetof(Path, source.y)                       },
	{ RFFT_UINT16, offsetof(Path, dest.x)                         },
	{ RFFT_UINT16, offsetof(Path, dest.y)                         },
	{ RFFT_UINT8,  offsetof(Path, dest_z)                         },
	{ RFFT_UINT8,  offsetof(Path, start_z)                        },
	{ RFFT_UINT8,  offsetof(Path, num_steps)                      },
	{ RFFT_UINT8,  offsetof(Path, curr_step)                      },
	{ RFFT_BIN(NUM_PATH_STEPS / 4), offsetof(Path, moves)         },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the anims table in a resfile (15-byte version 11
// structure).
const ResLayout AnimV11Layout = {
    15,                  // size on disc
    sizeof(AnimListing), // size in memory
    LAYOUT_FLAG_ARRAY,   // flags
    {
	{ RFFT_UINT16, offsetof(AnimListing, id)        },
	{ RFFT_UINT8,  offsetof(AnimListing, flags)     },
	{ RFFT_UINT16, offsetof(AnimListing, cbtype)    },
	{ RFFT_UINT32, offsetof(AnimListing, callback)  },
	{ RFFT_INTPTR, offsetof(AnimListing, user_data) },
	{ RFFT_UINT16, offsetof(AnimListing, speed)     },
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the anims table in a resfile (16-byte version 12
// structure).
// The AnimListing struct wasn't given a specific packing. This is my best
// guess at a 32-bit one. Savefile compatibility may be a bit dodgy.
const ResLayout AnimV12Layout = {
    20,                  // size on disc
    sizeof(AnimListing), // size in memory
    LAYOUT_FLAG_ARRAY,   // flags
    {
	{ RFFT_UINT16, offsetof(AnimListing, id)        },
	{ RFFT_UINT8,  offsetof(AnimListing, flags)     },
	{ RFFT_PAD,    1 }, // alignment
	{ RFFT_UINT16, offsetof(AnimListing, cbtype)    },
	{ RFFT_PAD,    2 }, // alignment
	{ RFFT_UINT32, offsetof(AnimListing, callback)  },
	{ RFFT_INTPTR, offsetof(AnimListing, user_data) },
	{ RFFT_UINT16, offsetof(AnimListing, speed)     },
	{ RFFT_PAD,    2 }, // alignment
	{ RFFT_END,    0 }
    }
};

// Describe the layout of the height semaphores table in a resfile.
const ResLayout HeightSemaphoreLayout = {
    4,                       // size on disc
    sizeof(height_semaphor), // size in memory
    LAYOUT_FLAG_ARRAY,       // flags
    {
	{ RFFT_UINT8,  offsetof(height_semaphor, x)         },
	{ RFFT_UINT8,  offsetof(height_semaphor, y)         },
	{ RFFT_UINT8,  offsetof(height_semaphor, floor_key) },
	{ RFFT_UINT8,  offsetof(height_semaphor, inuse)     },
	{ RFFT_END,    0 }
    }
};

// Version 11 level archives table.
const ResourceFormat LevelVersion11Format[MAX_LEVEL_INDEX+1] = {
    RES_FORMAT(U32Layout),            // 02 map version number
    RES_FORMAT(U32Layout),            // 03 object version number
    RES_FORMAT(FullMapLayout),        // 04 fullmap info
    RES_FORMAT(MapElemLayout),        // 05 map tiles (MapElem)
    { NULL, NULL, 0, NULL},           // 06 FIXME placeholder for schedules
    RES_FORMAT(TextureInfoLayout),    // 07 loved textures
    RES_FORMAT(ObjV11Layout),         // 08 objects
    RES_FORMAT(ObjRefLayout),         // 09 objrefs
    RES_FORMAT(GunLayout),            // 10 gun object info
    RES_FORMAT(AmmoLayout),           // 11 ammo object info
    RES_FORMAT(PhysicsLayout),        // 12 physics object info
    RES_FORMAT(GrenadeLayout),        // 13 grenade object into
    RES_FORMAT(DrugLayout),           // 14 drug object info
    RES_FORMAT(HardwareV11Layout),    // 15 hardware object info
    RES_FORMAT(SoftwareV11Layout),    // 16 software object info
    RES_FORMAT(BigStuffLayout),       // 17 bigstuff object info
    RES_FORMAT(SmallStuffLayout),     // 18 smallstuff object info
    RES_FORMAT(FixtureLayout),        // 19 fixture object info
    RES_FORMAT(DoorLayout),           // 20 door object info
    RES_FORMAT(AnimatingLayout),      // 21 animating object info
    RES_FORMAT(TrapLayout),           // 22 trap object info
    RES_FORMAT(ContainerV11Layout),   // 23 container object info
    RES_FORMAT(CritterLayout),        // 24 critter object info
    RES_FORMAT(GunLayout),            // 25 default gun (as 10 but single entry)
    RES_FORMAT(AmmoLayout),           // 26 default ammo
    RES_FORMAT(PhysicsLayout),        // 27 default physics
    RES_FORMAT(GrenadeLayout),        // 28 default grenade
    RES_FORMAT(DrugLayout),           // 29 default drug
    RES_FORMAT(HardwareV11Layout),    // 30 default hardware
    RES_FORMAT(SoftwareV11Layout),    // 31 default software
    RES_FORMAT(BigStuffLayout),       // 32 default bigstuff
    RES_FORMAT(SmallStuffLayout),     // 33 default smallstuff
    RES_FORMAT(FixtureLayout),        // 34 default fixture
    RES_FORMAT(DoorLayout),           // 35 default door
    RES_FORMAT(AnimatingLayout),      // 36 default animating
    RES_FORMAT(TrapLayout),           // 37 default trap
    RES_FORMAT(ContainerV11Layout),   // 38 default container
    RES_FORMAT(CritterLayout),        // 39 default critter
    RES_FORMAT(U32Layout),            // 40 misc version (not used)
    { NULL, NULL, 0, NULL },          // 41 not used
    RES_FORMAT(AnimTextureV11Layout), // 42 anim textures
    RES_FORMAT(HackCameraLayout),     // 43 hack camera objects
    RES_FORMAT(HackCameraLayout),     // 44 hack camera surrogates
    RES_FORMAT(LevelDataV11Layout),   // 45 level data
    { NULL, NULL, 0, NULL },          // 46 map strings (character array)
    RES_FORMAT(U32Layout),            // 47 map magic (next available offset)
    { NULL, NULL, 0, NULL },          // 48 not used
    RES_FORMAT(PathLayout),           // 49 paths
    RES_FORMAT(U16Layout),            // 50 used paths
    RES_FORMAT(AnimV11Layout),        // 51 anim list
    RES_FORMAT(U16Layout),            // 52 anim counter
    RES_FORMAT(HeightSemaphoreLayout) // 53 semaphores
};

// Version 12 level archives table.
const ResourceFormat LevelVersion12Format[MAX_LEVEL_INDEX+1] = {
    RES_FORMAT(U32Layout),            // 02 map version number
    RES_FORMAT(U32Layout),            // 03 object version number
    RES_FORMAT(FullMapLayout),        // 04 fullmap info
    RES_FORMAT(MapElemLayout),        // 05 map tiles (MapElem)
    { NULL, NULL, 0, NULL},           // 06 FIXME placeholder for schedules
    RES_FORMAT(TextureInfoLayout),    // 07 loved textures
    RES_FORMAT(ObjV12Layout),         // 08 objects
    RES_FORMAT(ObjRefLayout),         // 09 objrefs
    RES_FORMAT(GunLayout),            // 10 gun object info
    RES_FORMAT(AmmoLayout),           // 11 ammo object info
    RES_FORMAT(PhysicsLayout),        // 12 physics object info
    RES_FORMAT(GrenadeLayout),        // 13 grenade object into
    RES_FORMAT(DrugLayout),           // 14 drug object info
    RES_FORMAT(HardwareV12Layout),    // 15 hardware object info
    RES_FORMAT(SoftwareV12Layout),    // 16 software object info
    RES_FORMAT(BigStuffLayout),       // 17 bigstuff object info
    RES_FORMAT(SmallStuffLayout),     // 18 smallstuff object info
    RES_FORMAT(FixtureLayout),        // 19 fixture object info
    RES_FORMAT(DoorLayout),           // 20 door object info
    RES_FORMAT(AnimatingLayout),      // 21 animating object info
    RES_FORMAT(TrapLayout),           // 22 trap object info
    RES_FORMAT(ContainerV12Layout),   // 23 container object info
    RES_FORMAT(CritterLayout),        // 24 critter object info
    RES_FORMAT(GunLayout),            // 25 default gun (as 10 but single entry)
    RES_FORMAT(AmmoLayout),           // 26 default ammo
    RES_FORMAT(PhysicsLayout),        // 27 default physics
    RES_FORMAT(GrenadeLayout),        // 28 default grenade
    RES_FORMAT(DrugLayout),           // 29 default drug
    RES_FORMAT(HardwareV12Layout),    // 30 default hardware
    RES_FORMAT(SoftwareV12Layout),    // 31 default software
    RES_FORMAT(BigStuffLayout),       // 32 default bigstuff
    RES_FORMAT(SmallStuffLayout),     // 33 default smallstuff
    RES_FORMAT(FixtureLayout),        // 34 default fixture
    RES_FORMAT(DoorLayout),           // 35 default door
    RES_FORMAT(AnimatingLayout),      // 36 default animating
    RES_FORMAT(TrapLayout),           // 37 default trap
    RES_FORMAT(ContainerV12Layout),   // 38 default container
    RES_FORMAT(CritterLayout),        // 39 default critter
    RES_FORMAT(U32Layout),            // 40 misc version (not used)
    { NULL, NULL, 0, NULL },          // 41 not used
    RES_FORMAT(AnimTextureV12Layout), // 42 anim textures
    RES_FORMAT(HackCameraLayout),     // 43 hack camera objects
    RES_FORMAT(HackCameraLayout),     // 44 hack camera surrogates
    RES_FORMAT(LevelDataV12Layout),   // 45 level data
    { NULL, NULL, 0, NULL },          // 46 map strings (character array)
    RES_FORMAT(U32Layout),            // 47 map magic (next available offset)
    { NULL, NULL, 0, NULL },          // 48 not used
    RES_FORMAT(PathLayout),           // 49 paths
    RES_FORMAT(U16Layout),            // 50 used paths
    RES_FORMAT(AnimV12Layout),        // 51 anim list
    RES_FORMAT(U16Layout),            // 52 anim counter
    RES_FORMAT(HeightSemaphoreLayout) // 53 semaphores
};
