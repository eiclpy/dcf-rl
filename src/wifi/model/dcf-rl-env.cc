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

#include "dcf-rl-env.h"
#include "ns3/boolean.h"

namespace ns3 {
GlobalValue gUseRl = GlobalValue ("UseRl", "UseRl", BooleanValue (false), MakeBooleanChecker ());
DcfRl::DcfRl (uint16_t id) : Ns3AIRL<DcfRlEnv, DcfRlAct> (id)
{
  SetCond (2, 0);
}

bool
DcfRl::step (Ptr<WifiPhy> phy)
{
  auto env = EnvSetterCond ();
  // for (uint32_t i = 0; i < m_phys.size (); i++)
  //   {
  //     if (m_phys[i])
  //       {
  //         env->flag[i] = true;
  //         env->power[i] = m_phys[i]->m_interference.m_firstPower;
  //       }
  //   }
  env->time = Simulator::Now ().GetMicroSeconds ();
  env->power = phy->m_interference.m_firstPower;
  env->idx = phy->GetWifiUid ();
  SetCompleted ();
  auto act = ActionGetterCond ();
  // for (uint32_t i = 0; i < m_phys.size (); i++)
  //   {
  //     NS_UNUSED (act->ccaBusy[i]);
  //   }
  bool ret = act->ccaBusy;
  GetCompleted ();
  return ret;
}

Ptr<DcfRl> gDcfRl = Create<DcfRl> (1357);
} // namespace ns3
