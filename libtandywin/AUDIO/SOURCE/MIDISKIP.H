#ifndef MIDISKIP_H
#define MIDISKIP_H
#include "MIDIMSG.H"
/* Ordinary SMF traffic the PSG renderer deliberately does not model:
   channel/key aftertouch and controllers outside the MIDIMSG.H list
   (RPN/NRPN, data entry, filters, ...). The producer drops them so such
   files still open. Melodic notes 45..127, drum notes 35..81, program,
   bend and the MIDIMSG.H controllers are played. Notes outside those
   ranges, nonzero banks and anything else MSValidMessage refuses are
   still refused, never skipped. */
static LTP_U16 MSSkipMessage(LTP_U32 msg)
{
 LTP_U16 s=(LTP_U16)msg&240,a=((LTP_U16)msg>>8)&255;
 if(MSValidMessage(msg,0))return 0;
 if(s==0xa0||s==0xd0)return 1;
 return s==0xb0&&a!=0&&a!=32;
}
#endif
