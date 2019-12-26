// Note no include guard. Expected to be included more than once with different
// macros controlling the details of the structure.

// Expects PL_MFD_PUZZLE_SIZE to be set. The original DOS game had 32 bytes
// reserved for MFD puzzle status; this was later expanded to 64. Unfortunately,
// save game compatibility between versions wasn't really a thing and the
// structure version number wasn't changed, so we have to go by length.
// We can load both DOS and enhanced edition save files, but the DOS edition
// won't be able to load ours; the enhanced edition should.

// C preprocessor hackery to name the struct according to the size of the MFD
// access puzzles field. It will ultimately create structures named
// PlayerLayout_M32 and PlayerLayout_M64 according to the value of
// PL_MFD_PUZZLE_SIZE that is set on entry.
#define JOIN(x,y) x ## y
#define STRUCTNAME(x,y) JOIN(x,y)

// Describe the layout of the player in a resfile.
const ResLayout STRUCTNAME(PlayerLayout_M, PL_MFD_PUZZLE_SIZE) =
{
  711 + PL_MFD_PUZZLE_SIZE + 654, // size on disc
  sizeof(Player), // size in memory
  0,              // flags
  {
    { RFFT_BIN(20), offsetof(Player, name)               },  //char [20]
    { RFFT_UINT8,   offsetof(Player, realspace_level)    },  //char
    { RFFT_BIN(4),  offsetof(Player, difficulty)         },  //byte [4]
    { RFFT_BIN(3),  offsetof(Player, level_diff_zorched) },  //ubyte [3]
    { RFFT_UINT32,  offsetof(Player, game_time)          },  //uint32_t
    { RFFT_UINT32,  offsetof(Player, last_second_update) },  //uint32_t
    { RFFT_UINT32,  offsetof(Player, last_drug_update)   },  //uint32_t
    { RFFT_UINT32,  offsetof(Player, last_ware_update)   },  //uint32_t
    { RFFT_UINT32,  offsetof(Player, last_anim_check)    },  //uint32_t
    { RFFT_UINT32,  offsetof(Player, queue_time)         },  //int
    { RFFT_UINT32,  offsetof(Player, deltat)             },  //int
    { RFFT_UINT8,   offsetof(Player, detail_level)       },  //byte
    { RFFT_UINT8,   offsetof(Player, level)              },  //ubyte

    //short initial_shodan_vals[22];
    #define L(x)  { RFFT_UINT16, offsetof(Player, initial_shodan_vals[x]) }
    L(0),L(1),L(2),L(3),L(4),L(5),L(6),L(7),L(8),L(9),L(10),L(11),L(12),L(13),L(14),L(15),L(16),L(17),L(18),L(19),
    L(20),L(21),
    #undef L

    { RFFT_BIN(6), offsetof(Player, controls)        },  //byte [6]
    { RFFT_UINT16, offsetof(Player, rep)             },  //short
    { RFFT_UINT16, offsetof(Player, realspace_loc.x) },  //ushort
    { RFFT_UINT16, offsetof(Player, realspace_loc.y) },  //ushort
    { RFFT_UINT8,  offsetof(Player, realspace_loc.z) },  //ubyte
    { RFFT_UINT8,  offsetof(Player, realspace_loc.p) },  //ubyte
    { RFFT_UINT8,  offsetof(Player, realspace_loc.h) },  //ubyte
    { RFFT_UINT8,  offsetof(Player, realspace_loc.b) },  //ubyte
    { RFFT_UINT32, offsetof(Player, version_num)     },  //int

    //short inventory[14];
    #define L(x)  { RFFT_UINT16, offsetof(Player, inventory[x]) }
    L(0),L(1),L(2),L(3),L(4),L(5),L(6),L(7),L(8),L(9),L(10),L(11),L(12),L(13),
    #undef L

    { RFFT_UINT8,   offsetof(Player, posture)          },  //ubyte
    { RFFT_UINT8,   offsetof(Player, foot_planted)     },  //uchar
    { RFFT_UINT8,   offsetof(Player, leanx)            },  //byte
    { RFFT_UINT8,   offsetof(Player, leany)            },  //byte
    { RFFT_UINT16,  offsetof(Player, eye)              },  //uint16_t
    { RFFT_UINT8,   offsetof(Player, hit_points)       },  //ubyte
    { RFFT_UINT8,   offsetof(Player, cspace_hp)        },  //ubyte
    { RFFT_UINT16,  offsetof(Player, hit_points_regen) },  //ushort
    { RFFT_BIN(8),  offsetof(Player, hit_points_lost)  },  //ubyte [8]
    { RFFT_UINT16,  offsetof(Player, bio_post_expose)  },  //ushort
    { RFFT_UINT16,  offsetof(Player, rad_post_expose)  },  //ushort
    { RFFT_UINT8,   offsetof(Player, energy)           },  //ubyte
    { RFFT_UINT8,   offsetof(Player, energy_spend)     },  //ubyte
    { RFFT_UINT8,   offsetof(Player, energy_regen)     },  //ubyte
    { RFFT_UINT8,   offsetof(Player, energy_out)       },  //uchar
    { RFFT_UINT16,  offsetof(Player, cspace_trips)     },  //short
    { RFFT_UINT32,  offsetof(Player, cspace_time_base) },  //int
    { RFFT_BIN(64), offsetof(Player, questbits)        },  //ubyte [64]

    //short questvars[64];
    #define L(x)  { RFFT_UINT16, offsetof(Player, questvars[x]) }
    L(0),L(1),L(2),L(3),L(4),L(5),L(6),L(7),L(8),L(9),L(10),L(11),L(12),L(13),L(14),L(15),L(16),L(17),L(18),L(19),
    L(20),L(21),L(22),L(23),L(24),L(25),L(26),L(27),L(28),L(29),L(30),L(31),L(32),L(33),L(34),L(35),L(36),L(37),
    L(38),L(39),L(40),L(41),L(42),L(43),L(44),L(45),L(46),L(47),L(48),L(49),L(50),L(51),L(52),L(53),L(54),L(55),
    L(56),L(57),L(58),L(59),L(60),L(61),L(62),L(63),
    #undef L

    { RFFT_UINT32,    offsetof(Player, hud_modes)          },  //uint32_t
    { RFFT_UINT8,     offsetof(Player, experience)         },  //uchar
    { RFFT_UINT32,    offsetof(Player, fatigue)            },  //int
    { RFFT_UINT16,    offsetof(Player, fatigue_spend)      },  //ushort
    { RFFT_UINT16,    offsetof(Player, fatigue_regen)      },  //ushort
    { RFFT_UINT16,    offsetof(Player, fatigue_regen_base) },  //ushort
    { RFFT_UINT16,    offsetof(Player, fatigue_regen_max)  },  //ushort
    { RFFT_UINT8,     offsetof(Player, accuracy)           },  //byte
    { RFFT_UINT8,     offsetof(Player, shield_absorb_rate) },  //ubyte
    { RFFT_UINT8,     offsetof(Player, shield_threshold)   },  //ubyte
    { RFFT_UINT8,     offsetof(Player, light_value)        },  //ubyte
    { RFFT_BIN(2*5),  offsetof(Player, mfd_virtual_slots)  },  //ubyte [2][5]
    { RFFT_BIN(7),    offsetof(Player, mfd_slot_status)    },  //uint8_t [7]
    { RFFT_BIN(7),    offsetof(Player, mfd_all_slots)      },  //ubyte [7]
    { RFFT_BIN(32),   offsetof(Player, mfd_func_status)    },  //ubyte [32]
    { RFFT_BIN(32*8), offsetof(Player, mfd_func_data)      },  //ubyte [32][8]
    { RFFT_BIN(2),    offsetof(Player, mfd_current_slots)  },  //ubyte [2]
    { RFFT_BIN(2),    offsetof(Player, mfd_empty_funcs)    },  //ubyte [2]
    { RFFT_BIN(PL_MFD_PUZZLE_SIZE),
                      offsetof(Player, mfd_access_puzzles) },  //uchar [64]
    { RFFT_BIN(2),    offsetof(Player, mfd_save_slot)      },  //char [2]
    { RFFT_BIN(15),   offsetof(Player, hardwarez)          },  //ubyte [15]
    { RFFT_BIN(7),    offsetof(Player, softs.combat)       },  //ubyte [7]
    { RFFT_BIN(3),    offsetof(Player, softs.defense)      },  //ubyte [3]
    { RFFT_BIN(9),    offsetof(Player, softs.misc)         },  //ubyte [9]
    { RFFT_BIN(15),   offsetof(Player, cartridges)         },  //ubyte [15]
    { RFFT_BIN(15),   offsetof(Player, partial_clip)       },  //ubyte [15]
    { RFFT_BIN(7),    offsetof(Player, drugs)              },  //ubyte [7]
    { RFFT_BIN(7),    offsetof(Player, grenades)           },  //ubyte [7]
    { RFFT_BIN(294),  offsetof(Player, email)              },  //uchar [294]
    { RFFT_BIN(14),   offsetof(Player, logs)               },  //ubyte [14]

    //weapon_slot weapons[7];
    #define L(x)  { RFFT_UINT8, offsetof(Player, weapons[x].type)      }, \
                  { RFFT_UINT8, offsetof(Player, weapons[x].subtype)   }, \
                  { RFFT_UINT8, offsetof(Player, weapons[x].ammo)      }, \
                  { RFFT_UINT8, offsetof(Player, weapons[x].ammo_type) }, \
                  { RFFT_UINT8, offsetof(Player, weapons[x].make_info) }
    L(0),L(1),L(2),L(3),L(4),L(5),L(6),
    #undef L

    { RFFT_BIN(15), offsetof(Player, hardwarez_status)        },  //ubyte [15]
    { RFFT_BIN(7),  offsetof(Player, softs_status.combat)     },  //ubyte [7]
    { RFFT_BIN(3),  offsetof(Player, softs_status.defense)    },  //ubyte [3]
    { RFFT_BIN(9),  offsetof(Player, softs_status.misc)       },  //ubyte [9]
    { RFFT_UINT8,   offsetof(Player, jumpjet_energy_fraction) },  //ubyte
    { RFFT_BIN(32), offsetof(Player, email_sender_counts)     },  //ubyte [32]
    { RFFT_BIN(7),  offsetof(Player, drug_status)             },  //byte [7]
    { RFFT_BIN(7),  offsetof(Player, drug_intensity)          },  //ubyte [7]

    //ushort grenades_time_setting[7];
    #define L(x)  { RFFT_UINT16, offsetof(Player, grenades_time_setting[x]) }
    L(0),L(1),L(2),L(3),L(4),L(5),L(6),
    #undef L

    { RFFT_UINT16,  offsetof(Player, time2dest)       },  //ushort
    { RFFT_UINT16,  offsetof(Player, time2comp)       },  //ushort
    { RFFT_UINT16,  offsetof(Player, curr_target)     },  //short
    { RFFT_UINT32,  offsetof(Player, last_fire)       },  //uint32_t
    { RFFT_UINT16,  offsetof(Player, fire_rate)       },  //ushort
    { RFFT_BIN(10), offsetof(Player, actives)         },  //ubyte [10]
    { RFFT_UINT16,  offsetof(Player, save_obj_cursor) },  //short
    { RFFT_UINT16,  offsetof(Player, panel_ref)       },  //short
    { RFFT_UINT32,  offsetof(Player, num_victories)   },  //int
    { RFFT_UINT32,  offsetof(Player, time_in_cspace)  },  //int
    { RFFT_UINT32,  offsetof(Player, rounds_fired)    },  //int
    { RFFT_UINT32,  offsetof(Player, num_hits)        },  //int
    { RFFT_UINT32,  offsetof(Player, num_deaths)      },  //int
    { RFFT_UINT32,  offsetof(Player, eye_pos)         },  //int32_t

    //int32_t edms_state[12];
    #define L(x)  { RFFT_UINT32, offsetof(Player, edms_state[x]) }
    L(0),L(1),L(2),L(3),L(4),L(5),L(6),L(7),L(8),L(9),L(10),L(11),
    #undef L

    { RFFT_UINT8,  offsetof(Player, current_active)     },  //ubyte
    { RFFT_UINT8,  offsetof(Player, active_bio_tracks)  },  //ubyte
    { RFFT_UINT16, offsetof(Player, current_email)      },  //short
    { RFFT_BIN(6), offsetof(Player, version)            },  //char [6]
    { RFFT_UINT8,  offsetof(Player, dead)               },  //uchar
    { RFFT_UINT16, offsetof(Player, lean_filter_state)  },  //ushort
    { RFFT_UINT16, offsetof(Player, FREE_BITS_HERE)     },  //ushort
    { RFFT_UINT8,  offsetof(Player, mfd_save_vis)       },  //uchar
    { RFFT_UINT32, offsetof(Player, auto_fire_click)    },  //uint32_t
    { RFFT_UINT32, offsetof(Player, posture_slam_state) },  //uint32_t
    { RFFT_UINT8,  offsetof(Player, terseness)          },  //uchar
    { RFFT_UINT32, offsetof(Player, last_bob)           },  //uint32_t
    { RFFT_BIN(9), offsetof(Player, pad)                },  //uchar [9]

    { RFFT_END, 0 }
  }
};

#undef JOIN
#undef STRUCTNAME
