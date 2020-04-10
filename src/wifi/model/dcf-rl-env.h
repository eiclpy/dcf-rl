/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Huazhong University of Science and Technology, Dian Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Pengyu Liu <eic_lpy@hust.edu.cn>
 */
#pragma once
#include "ns3/global-value.h"
#include "ns3/ns3-ai-rl.h"
#include "wifi-phy.h"

namespace ns3 {
struct DcfRlEnv
{
  int64_t time;
  double power;
  uint32_t idx;
};
struct DcfRlAct
{
  bool ccaBusy;
};

class DcfRl : public Ns3AIRL<DcfRlEnv, DcfRlAct>
{
  DcfRl (void) = delete;

public:
  DcfRl (uint16_t id);
  bool step (Ptr<WifiPhy> phy);
};

extern Ptr<DcfRl> gDcfRl;
extern GlobalValue gUseRl;
} // namespace ns3
