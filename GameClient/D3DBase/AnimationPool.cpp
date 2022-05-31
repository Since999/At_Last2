#include "AnimationPool.h"
#include "Animation.h"
#include "ModelLoader.h"
#include "Configuration.h"

CAnimationPool::CAnimationPool()
{

}

Animation* CAnimationPool::GetAnimation(const std::string& name)
{
	auto found = animations.find(name);
	//if found one, return that animation pointer
	if (found != animations.end()) return &(*found).second;

	//else
	animations.emplace(name, ModelLoader::LoadAnimation(CConfiguration::MakePath(name, ANIMATION_DIR)));
	return GetAnimation(name);
}