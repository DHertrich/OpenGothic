#include "worldsound.h"

#include <Tempest/SoundEffect>

#include "game/gamesession.h"
#include "world/npc.h"
#include "gamemusic.h"
#include "world.h"
#include "gothic.h"
#include "resources.h"

using namespace Tempest;

const float WorldSound::maxDist   = 3500; // 35 meters
const float WorldSound::talkRange = 800;

bool WorldSound::Zone::checkPos(float x, float y, float z) const {
  return
      bbox[0].x <= x && x<bbox[1].x &&
      bbox[0].y <= y && y<bbox[1].y &&
      bbox[0].z <= z && z<bbox[1].z;
  }

WorldSound::WorldSound(Gothic& gothic, GameSession &game, World& owner)
  :gothic(gothic),game(game),owner(owner) {
  plPos = {-1000000,-1000000,-1000000};
  }

void WorldSound::setDefaultZone(const ZenLoad::zCVobData &vob) {
  def.bbox[0] = vob.bbox[0];
  def.bbox[1] = vob.bbox[1];
  def.name    = vob.vobName;
  }

void WorldSound::addZone(const ZenLoad::zCVobData &vob) {
  Zone z;
  z.bbox[0] = vob.bbox[0];
  z.bbox[1] = vob.bbox[1];
  z.name    = vob.vobName;

  zones.emplace_back(std::move(z));
  }

void WorldSound::addSound(const ZenLoad::zCVobData &vob) {
  auto& pr  = vob.zCVobSound;
  auto  snd = game.loadSoundFx(pr.sndName.c_str());
  if(snd==nullptr)
    return;

  WSound s{std::move(*snd)};
  s.eff = game.loadSound(s.proto);
  if(s.eff.isEmpty())
    return;

  s.eff.setPosition(vob.position.x,vob.position.y,vob.position.z);
  s.eff.setMaxDistance(pr.sndRadius);
  s.eff.setRefDistance(0);
  s.eff.setVolume(0.5f);

  s.loop     = pr.sndType==ZenLoad::SoundMode::SM_LOOPING;
  s.active   = pr.sndStartOn;
  s.delay    = uint64_t(pr.sndRandDelay*1000);
  s.delayVar = uint64_t(pr.sndRandDelayVar*1000);

  if(vob.vobType==ZenLoad::zCVobData::VT_zCVobSoundDaytime) {
    float b = vob.zCVobSoundDaytime.sndStartTime;
    float e = vob.zCVobSoundDaytime.sndEndTime;

    s.sndStart = gtime(int(b),int(b*60)%60);
    s.sndEnd   = gtime(int(e),int(e*60)%60);

    s.eff2 = game.loadSound(s.proto);
    s.eff2.setPosition(vob.position.x,vob.position.y,vob.position.z);
    s.eff2.setMaxDistance(pr.sndRadius);
    s.eff2.setRefDistance(0);
    s.eff2.setVolume(0.5f);
    } else {
    s.sndStart = gtime(0,0);
    s.sndEnd   = gtime(24,0);
    }

  worldEff.emplace_back(std::move(s));
  }

void WorldSound::emitSound(const char* s, float x, float y, float z, float range, bool fSlot) {
  if(range<=0.f)
    range = 3500.f;

  GSoundEffect* slot = nullptr;
  if(isInListenerRange({x,y,z},range)) {
    std::lock_guard<std::mutex> guard(sync);
    if(fSlot)
      slot = &freeSlot[s];
    if(slot!=nullptr && !slot->isFinished())
      return;
    auto snd = game.loadSoundFx(s);
    if(snd==nullptr)
      return;
    GSoundEffect eff = game.loadSound(*snd);
    if(eff.isEmpty())
      return;
    eff.setPosition(x,y,z);
    eff.setMaxDistance(maxDist);
    eff.setRefDistance(range);
    eff.play();
    tickSlot(eff);
    if(slot)
      *slot = std::move(eff); else
      effect.emplace_back(std::move(eff));
    }
  }

void WorldSound::emitSound3d(const char* s, float x, float y, float z, float range) {
  if(range<=0.f)
    range = 3500.f;

  auto snd = game.loadSoundFx(s);
  if(snd==nullptr)
    return;
  GSoundEffect eff = game.loadSound(*snd);
  if(eff.isEmpty())
    return;
  eff.setPosition(x,y,z);
  eff.setMaxDistance(maxDist);
  eff.setRefDistance(range);
  eff.play();

  std::lock_guard<std::mutex> guard(sync);
  tickSlot(eff);
  effect3d.emplace_back(std::move(eff));
  }

void WorldSound::emitSoundRaw(const char *s, float x, float y, float z, float range, bool fSlot) {
  if(range<=0.f)
    range = 3500.f;

  GSoundEffect* slot = nullptr;
  if(isInListenerRange({x,y,z},range)){
    std::lock_guard<std::mutex> guard(sync);
    if(fSlot)
      slot = &freeSlot[s];
    if(slot!=nullptr && !slot->isFinished())
      return;
    auto snd = game.loadSoundWavFx(s);
    if(snd==nullptr)
      return;
    GSoundEffect eff = game.loadSound(*snd);
    if(eff.isEmpty())
      return;
    eff.setPosition(x,y,z);
    eff.setMaxDistance(maxDist);
    eff.setRefDistance(range);
    eff.play();
    tickSlot(eff);
    if(slot)
      *slot = std::move(eff); else
      effect.emplace_back(std::move(eff));
    }
  }

void WorldSound::emitDlgSound(const char *s, float x, float y, float z, float range, uint64_t& timeLen) {
  if(isInListenerRange({x,y,z},range)){
    std::lock_guard<std::mutex> guard(sync);
    auto snd = Resources::loadSoundBuffer(s);
    if(snd.isEmpty())
      return;
    Tempest::SoundEffect eff = game.loadSound(snd);
    if(eff.isEmpty())
      return;
    eff.setPosition(x,y+180,z);
    eff.setMaxDistance(maxDist);
    eff.setRefDistance(range);
    eff.play();
    timeLen = eff.timeLength();
    effect.emplace_back(std::move(eff));
    }
  }

void WorldSound::takeSoundSlot(GSoundEffect &&eff) {
  if(eff.isFinished())
    return;
  effect.emplace_back(std::move(eff));
  }

void WorldSound::tick(Npc &player) {
  std::lock_guard<std::mutex> guard(sync);
  plPos = player.position();

  game.updateListenerPos(player);

  for(size_t i=0;i<effect.size();) {
    if(effect[i].isFinished()){
      effect[i]=std::move(effect.back());
      effect.pop_back();
      } else {
      tickSlot(effect[i]);
      ++i;
      }
    }

  for(size_t i=0;i<effect3d.size();) {
    if(effect3d[i].isFinished()){
      effect3d[i]=std::move(effect3d.back());
      effect3d.pop_back();
      } else {
      tickSlot(effect3d[i]);
      ++i;
      }
    }

  for(auto& i:freeSlot) {
    tickSlot(i.second);
    }

  for(auto& i:worldEff) {
    if(i.active && i.eff.isFinished() && (i.restartTimeout<owner.tickCount() || i.loop)){
      if(i.restartTimeout!=0) {
        auto time = owner.time();
        time = gtime(0,time.hour(),time.minute());
        if(i.sndStart<= time && time<i.sndEnd){
          i.eff.play();
          } else {
          if(!i.eff2.isEmpty())
            i.eff2.play();
          }
        }
      i.restartTimeout = owner.tickCount() + i.delay;
      if(i.delayVar>0)
        i.restartTimeout += uint64_t(std::rand())%i.delayVar;
      }
    }

  tickSoundZone(player);
  }

void WorldSound::tickSoundZone(Npc& player) {
  if(owner.tickCount()<nextSoundUpdate)
    return;
  nextSoundUpdate = owner.tickCount()+5*1000;

  Zone* zone=&def;
  if(currentZone!=nullptr &&
     currentZone->checkPos(plPos.x,plPos.y+player.translateY(),plPos.z)){
    zone = currentZone;
    } else {
    for(auto& z:zones) {
      if(z.checkPos(plPos.x,plPos.y+player.translateY(),plPos.z)) {
        zone = &z;
        }
      }
    }

  gtime           time  = owner.time().timeInDay();
  bool            isDay = (gtime(4,0)<=time && time<=gtime(21,0));
  bool            isFgt = owner.isTargeted(player);

  GameMusic::Tags mode  = GameMusic::Std;
  if(isFgt) {
    if(player.weaponState()==WeaponState::NoWeapon) {
      mode  = GameMusic::Thr;
      } else {
      mode = GameMusic::Fgt;
      }
    }
  GameMusic::Tags tags = GameMusic::mkTags(isDay ? GameMusic::Day : GameMusic::Ngt,mode);

  if(currentZone==zone && currentTags==tags)
    return;

  currentZone = zone;
  currentTags = tags;

  Zone*           zTry[]    = {zone, &def};
  GameMusic::Tags dayTry[]  = {isDay ? GameMusic::Day : GameMusic::Ngt, GameMusic::Day};
  GameMusic::Tags modeTry[] = {mode, mode==GameMusic::Thr ? GameMusic::Fgt : GameMusic::Std, GameMusic::Std};

  // multi-fallback strategy
  for(auto zone:zTry)
    for(auto day:dayTry)
      for(auto mode:modeTry) {
        const size_t sep = zone->name.find('_');
        const char*  tag = zone->name.c_str();
        if(sep!=std::string::npos)
          tag = tag+sep+1;

        tags = GameMusic::mkTags(day,mode);
        if(setMusic(tag,tags))
          return;
        }
  }

bool WorldSound::setMusic(const char* zone, GameMusic::Tags tags) {
  bool            isDay = (tags&GameMusic::Ngt)==0;
  const char*     smode = "STD";
  if(tags&GameMusic::Thr)
    smode = "THR";
  if(tags&GameMusic::Fgt)
    smode = "FGT";

  char name[64]={};
  std::snprintf(name,sizeof(name),"%s_%s_%s",zone,(isDay ? "DAY" : "NGT"),smode);

  if(auto* theme = gothic.getMusicDef(name)) {
    GameMusic::inst().setMusic(*theme,tags);
    return true;
    }
  return false;
  }

void WorldSound::tickSlot(GSoundEffect& slot) {
  if(slot.isFinished())
    return;
  auto  dyn = owner.physic();
  auto  pos = slot.position();
  float occ = dyn->soundOclusion(plPos.x,plPos.y+180/*head pos*/,plPos.z, pos.x,pos.y,pos.z);

  slot.setOcclusion(std::max(0.f,1.f-occ));
  }

bool WorldSound::isInListenerRange(const Tempest::Vec3& pos, float sndRgn) const {
  return (pos-plPos).quadLength()<4*(maxDist+sndRgn)*(maxDist+sndRgn);
  }

void WorldSound::aiOutput(const Tempest::Vec3& pos,const std::string &outputname) {
  if(isInListenerRange(pos,0)){
    std::lock_guard<std::mutex> guard(sync);
    game.emitGlobalSound(Resources::loadSoundBuffer(outputname+".wav"));
    }
  }
