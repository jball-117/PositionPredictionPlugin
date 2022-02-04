#define LINMATH_H //Conflicts with linmath.h if we done declare this here

#include "PositionPredictionPlugin.h"
#include "Hitbox.h"
//#include "CarManager.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "bakkesmod/wrappers/GameObject/BallWrapper.h"
#include "bakkesmod/wrappers/GameObject/CarWrapper.h"
#include "bakkesmod\wrappers\GameEvent\TutorialWrapper.h"
#include "bakkesmod/wrappers/arraywrapper.h"
#include "RenderingTools/Objects/Circle.h"
#include "RenderingTools/Objects/Frustum.h"
#include "RenderingTools/Objects/Line.h"
#include "RenderingTools/Objects/Sphere.h"
#include "RenderingTools/Extra/WrapperStructsExtensions.h"
#include "RenderingTools/Extra/RenderingMath.h"
#include <sstream>
#include "HTTPRequest.h"
#include <thread>
#include <iomanip>
#include <cmath>
#include <algorithm>

//BAKKESMOD_PLUGIN(PositionPredictionPlugin, "position prediction plugin", "1.0", PLUGINTYPE_FREEPLAY | PLUGINTYPE_CUSTOM_TRAINING)
BAKKESMOD_PLUGIN(PositionPredictionPlugin, "position prediction plugin", "1.0", PLUGINTYPE_FREEPLAY)
int global_render_limiter = 0;
//std::mutex lock;

PositionPredictionPlugin::PositionPredictionPlugin()
{

}

PositionPredictionPlugin::~PositionPredictionPlugin()
{
}

void PositionPredictionPlugin::onLoad()
{
	hitboxOn = std::make_shared<int>(0);
	cvarManager->registerCvar("cl_soccar_showhitbox", "0", "Show Hitbox", true, true, 0, true, 3).bindTo(hitboxOn);
	cvarManager->getCvar("cl_soccar_showhitbox").addOnValueChanged(std::bind(&PositionPredictionPlugin::OnHitboxOnValueChanged, this, std::placeholders::_1, std::placeholders::_2));

	hitboxColor = std::make_shared<LinearColor>(LinearColor{ 0.f,0.f,0.f,0.f });
	coordinates = Vector(0,0,0);
	cvarManager->registerCvar("cl_soccar_hitboxcolor", "#00ff22", "Color of the hitbox visualization.", true).bindTo(hitboxColor);

	//hitboxType = std::make_shared<int>(0);
	//cvarManager->registerCvar("cl_soccar_sethitboxtype", "0", "Set Hitbox Car Type", true, true, 0, true, 32767, false).bindTo(hitboxType);
	//cvarManager->getCvar("cl_soccar_sethitboxtype").addOnValueChanged(std::bind(&PositionPredictionPlugin::OnHitboxTypeChanged, this, std::placeholders::_1, std::placeholders::_2));


	gameWrapper->HookEvent("Function TAGame.Mutator_Freeplay_TA.Init", bind(&PositionPredictionPlugin::OnFreeplayLoad, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", bind(&PositionPredictionPlugin::OnFreeplayDestroy, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_TrainingEditor_TA.StartPlayTest", bind(&PositionPredictionPlugin::OnFreeplayLoad, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_TrainingEditor_TA.Destroyed", bind(&PositionPredictionPlugin::OnFreeplayDestroy, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function TAGame.GameInfo_Replay_TA.InitGame", bind(&PositionPredictionPlugin::OnFreeplayLoad, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function TAGame.Replay_TA.EventPostTimeSkip", bind(&PositionPredictionPlugin::OnFreeplayLoad, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function TAGame.GameInfo_Replay_TA.Destroyed", bind(&PositionPredictionPlugin::OnFreeplayDestroy, this, std::placeholders::_1));
	//gameWrapper->HookEvent("Function TAGame.Replay_TA.EventSpawned", [this](std::string eventName) {
	//	this->OnHitboxTypeChanged("", cvarManager->getCvar("cl_soccar_sethitboxtype"));
	//	});

	//cvarManager->registerNotifier("cl_soccar_listhitboxtypes", [this](std::vector<std::string> params) {
	//	cvarManager->log(CarManager::getHelpText());
	//	}, "List all hitbox integer types, use these values as parameters for cl_soccar_sethitboxtype", PERMISSION_ALL);

}

void PositionPredictionPlugin::OnFreeplayLoad(std::string eventName)
{
	// get the 8 hitbox points for current car type
	//hitboxes.clear();  // we'll reinitialize this in Render, for the first few ticks of free play, the car is null
	//cvarManager->log(std::string("OnFreeplayLoad") + eventName);
	if (*hitboxOn) {
		gameWrapper->RegisterDrawable(std::bind(&PositionPredictionPlugin::Render, this, std::placeholders::_1));
		//std::thread thread_obj(&GameWrapper::RegisterDrawable, gameWrapper, std::bind(&PositionPredictionPlugin::Render, this, std::placeholders::_1));
		//thread_obj.detach();
	}
}

void PositionPredictionPlugin::OnFreeplayDestroy(std::string eventName)
{
	gameWrapper->UnregisterDrawables();
}

void PositionPredictionPlugin::OnHitboxOnValueChanged(std::string oldValue, CVarWrapper cvar)
{
	int ingame = (gameWrapper->IsInReplay()) ? 2 : ((gameWrapper->IsInOnlineGame()) ? 0 : ((gameWrapper->IsInGame()) ? 1 : 0));
	//cvarManager->log("OnHitboxValueChanged: " + std::to_string(ingame));
	if (cvar.getIntValue() & ingame) {
		OnFreeplayLoad("Load");
	}
	else
	{
		OnFreeplayDestroy("Destroy");
	}
}

//void PositionPredictionPlugin::OnHitboxTypeChanged(std::string oldValue, CVarWrapper cvar) {
//	hitboxes.clear();
//}


#include <iostream>     // std::cout
#include <fstream> 

//Vector Rotate(Vector aVec, double roll, double yaw, double pitch)
//{
//
//	// this rotate is kind of messed up, because UE's xyz coordinates didn't match the axes i expected
//   /*
//   float sx = sin(pitch);
//   float cx = cos(pitch);
//   float sy = sin(yaw);
//   float cy = cos(yaw);
//   float sz = sin(roll);
//   float cz = cos(roll);
//   */
//	float sx = sin(roll);
//	float cx = cos(roll);
//	float sy = sin(yaw);
//	float cy = cos(yaw);
//	float sz = sin(pitch);
//	float cz = cos(pitch);
//
//	aVec = Vector(aVec.X, aVec.Y * cx - aVec.Z * sx, aVec.Y * sx + aVec.Z * cx);  //2  roll?
//
//
//	aVec = Vector(aVec.X * cz - aVec.Y * sz, aVec.X * sz + aVec.Y * cz, aVec.Z); //1   pitch?
//	aVec = Vector(aVec.X * cy + aVec.Z * sy, aVec.Y, -aVec.X * sy + aVec.Z * cy);  //3  yaw?
//
//	// ugly fix to change coordinates to Unreal's axes
//	float tmp = aVec.Z;
//	aVec.Z = aVec.Y;
//	aVec.Y = tmp;
//	return aVec;
//}


//void PositionPredictionPlugin::get_coordinates(std::string &data, Vector &coordinates) {
void get_coordinates(std::string data, Vector &coordinates) {
	// testing threading
	// SLEEPING DEFINITELY STOPS EVERYTHING IN GAME!
	//std::vector<float> tmp;
	//std::stringstream ss(data);
	//while (ss.good()) {
	//	std::string substr;
	//	std::getline(ss, substr, ',');
	//	tmp.push_back(std::stof(substr));
	//}
	//Vector ret; ret.X = tmp[0]; ret.Y = tmp[1]; ret.Z = tmp[2];
	//return ret;
	////////////////////

	//std::lock_guard<std::mutex> guard(lock);
	//cvarManager->log("sending request...\n");
	std::string coords;
	try
	{
		http::Request request{ "http://127.0.0.1:5000/analyze?data=" + data };

		// send a get request
		const auto response = request.send("GET");
		coords = std::string{ response.body.begin(), response.body.end() };
	}
	catch (const std::exception& e)
	{
		coords = "REQUEST FAILED, error: " + std::string(e.what()) + '\n';
	}
	//cvarManager->log("GOTO: " + coords + "\n");
	std::vector<float> tmp;
	std::stringstream ss(coords);
	while (ss.good()) {
		std::string substr;
		std::getline(ss, substr, ',');
		tmp.push_back(std::stof(substr));
	}
	//return Vector(tmp[0], tmp[1], tmp[2]);
	coordinates = Vector(tmp[0], tmp[1], tmp[2]);
}

void PositionPredictionPlugin::Render(CanvasWrapper canvas)
{	
	int ingame = (gameWrapper->IsInGame()) ? 1 : (gameWrapper->IsInReplay()) ? 2 : 0;
	if (*hitboxOn & ingame)
	{
		if (gameWrapper->IsInOnlineGame() && ingame) return;
		ServerWrapper game = (ingame == 1) ? gameWrapper->GetGameEventAsServer() : gameWrapper->GetGameEventAsReplay();
		if (game.IsNull()) return;
		auto camera = gameWrapper->GetCamera();
		if (camera.IsNull()) return;
		RT::Frustum frust{ canvas, camera };
		canvas.SetColor(*hitboxColor);
		///////////////////////////////////////////////////////////////////////////////////
		if (global_render_limiter < 100) {
			global_render_limiter++;
			RT::Sphere(coordinates, 100.f).Draw(canvas, frust, camera.GetLocation(), 16);
			return;
		}
		else {
			global_render_limiter = 0;
		}
		///////////////////////////////////////////////////////////////////////////////////
		ArrayWrapper<CarWrapper> cars = game.GetCars();
		if (cars.IsNull()) return;
		BallWrapper ball = game.GetBall();
		if (ball.IsNull()) return;
		cvarManager->log(typeid(ball).name());
		CarWrapper my_car = game.GetGameCar();
		if (my_car.IsNull()) return;
		cvarManager->log(typeid(my_car).name());
		BoostWrapper my_boost = my_car.GetBoostComponent();
		if (my_boost.IsNull()) return;
		float boost = my_boost.GetCurrentBoostAmount();
		std::string my_name = my_car.GetOwnerName();
		cvarManager->log(my_name);
		//std::vector<Vector> hitbox;
		//static int car_count = 0;
		//if (cars.Count() < hitboxes.size())
		//{
		//	hitboxes.clear();
		//}

		//int car_i = 0;
		for (auto car : cars) {
			if (car.IsNull()) continue;			
			std::string name = car.GetOwnerName();
			///////////////////////
			if (name == my_name) {
				continue;
			}
			///////////////////////

			//if (hitboxes.size() <= car_i) { // initialize hitboxes 
			//	hitboxes.push_back(CarManager::getCarPosition(static_cast<CARBODY>(*hitboxType), car));
			//}

			Vector vc = car.GetLocation();
			Vector my_v = my_car.GetLocation();
			Vector vb = ball.GetLocation();
			//std::string data = std::to_string(vc.X) + "%2C" + std::to_string(vc.Y) + "%2C" + std::to_string(vc.Z)
			//				   + "%2C" + std::to_string(boost) + "%2C" +
			//				   std::to_string(vb.X) + "%2C" + std::to_string(vb.Y) + "%2C" + std::to_string(vb.Z);
			
			// normalizing
			float tmp;
			tmp = std::round(vc.X * 10) * 0.1;
			std::string vcX = std::to_string(tmp);
			tmp = std::round(vc.Y* 10) * 0.1;
			std::string vcY = std::to_string(tmp);
			tmp = std::round(vc.Z * 10) * 0.1;
			std::string vcZ = std::to_string(tmp);
			tmp = std::round(my_v.X * 10) * 0.1;
			std::string myvX = std::to_string(tmp);
			tmp = std::round(my_v.Y * 10) * 0.1;
			std::string myvY = std::to_string(tmp);
			tmp = std::round(my_v.Z * 10) * 0.1;
			std::string myvZ = std::to_string(tmp);
			tmp = std::round(vb.X * 10) * 0.1;
			std::string vbX = std::to_string(tmp);
			tmp = std::round(vb.Y * 10) * 0.1;
			std::string vbY = std::to_string(tmp);
			tmp = std::round(vb.Z * 10) * 0.1;
			std::string vbZ = std::to_string(tmp);

			std::string data = vcX + "%2C" + vcY + "%2C" + vcZ + myvX + "%2C" + myvY + "%2C" + myvZ
							   + "%2C" + std::to_string(boost) + "%2C" + vbX + "%2C" + vbY + "%2C" + vbZ;
			
			//Rotator r = car.GetRotation();
			
			cvarManager->log("Gathererd data... " + data + "\n");
			// send coordinates to flask to get new coordinates
			//get_coordinates(data);
			std::thread thread_obj(get_coordinates, data, std::ref(coordinates));
			thread_obj.detach();
			cvarManager->log("Drawing...\n");
			RT::Sphere(coordinates, 100.f).Draw(canvas, frust, camera.GetLocation(), 16);
			cvarManager->log("Done Drawing.\n");

			//double dPitch = (double)r.Pitch / 32768.0 * 3.14159;
			//double dYaw = (double)r.Yaw / 32768.0 * 3.14159;
			//double dRoll = (double)r.Roll / 32768.0 * 3.14159;

			//Vector2F carLocation2D = canvas.ProjectF(v);
			////Vector2 hitbox2D[8];
			//Vector hitbox3D[8];

			//hitboxes.at(car_i).getPoints(hitbox);
			//if (fabs(hitbox[0].Z - hitbox[1].Z) < 0.01f)
			//{ // on the first tick this gets hooked, the extent/offset returned is still 0
			//  // so we skip this tick and throw the data away to get it again next frame
			//	hitboxes.clear();
			//	return;
			//}
			//for (int i = 0; i < 8; i++) {
			//	hitbox3D[i] = Rotate(hitbox[i], dRoll, -dYaw, dPitch) + v;
			//	//hitbox2D[i] = canvas.Project(Rotate(hitbox[i], dRoll, -dYaw, dPitch) + v);
			//}
			//RT::Line(hitbox3D[0], hitbox3D[1], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[1], hitbox3D[2], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[2], hitbox3D[3], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[3], hitbox3D[0], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[4], hitbox3D[5], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[5], hitbox3D[6], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[6], hitbox3D[7], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[7], hitbox3D[4], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[0], hitbox3D[4], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[1], hitbox3D[5], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[2], hitbox3D[6], 1.f).DrawWithinFrustum(canvas, frust);
			//RT::Line(hitbox3D[3], hitbox3D[7], 1.f).DrawWithinFrustum(canvas, frust);

			//float diff = (camera.GetLocation() - v).magnitude();
			//Quat car_rot = RotatorToQuat(r);
			//if (diff < 1000.f)
			//	RT::Sphere(v, car_rot, 2.f).Draw(canvas, frust, camera.GetLocation(), 10);


			//auto sim = car.GetVehicleSim();
			//auto wheels = sim.GetWheels();
			//if (wheels.IsNull()) continue;
			//Vector turn_axis = RotateVectorWithQuat(Vector{ 0.f, 0.f, 1.f }, car_rot);
			//Quat upright_rot = RT::AngleAxisRotation(3.14159f / 2.0f, Vector{ 1.f, 0.f, 0.f });
			//for (auto wheel : wheels)
			//{
			//	Vector loc = wheel.GetLocalRestPosition() - Vector(0.f, 0.f, wheel.GetSuspensionDistance());
			//	loc = RotateVectorWithQuat(loc, car_rot);
			//	loc = loc + v;

			//	Quat turn_rot = RT::AngleAxisRotation(wheel.GetSteer2(), Vector{ 0.f, 0.f, 1.f });
			//	Quat final_rot = car_rot * turn_rot * upright_rot;

			//	RT::Circle circ{ loc, final_rot, wheel.GetWheelRadius() };

			//	circ.Draw(canvas, frust);
			//}

			//car_i++;
		}
	}
}

void PositionPredictionPlugin::onUnload()
{
}
