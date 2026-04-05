#include "../util.h"
#include "raylib.h"
#include "../physics/physics.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

void checkChair(player& player, gameData& gData,Sound chairSound, Sound chairScared, meshedObject& chairEntity) {
    if (player.pEntity.groundPlane == 64 || player.pEntity.groundPlane == 47 || player.pEntity.groundPlane == 68 || player.pEntity.groundPlane == 43) {
        if (gData.hasAudioLevel1 == false) {
            gData.hasAudioLevel1 = true;
            gData.hasChairScared = true;
            PlaySound(chairSound);
            player.canMove = false;
            std::thread([&player, &gData]() {
                std::this_thread::sleep_for(std::chrono::seconds(7));
                gData.hasChairScared = false;
                player.canMove = true;
            }).detach();
            // chairEntity.pEntity.magnitude.y = 25.f;
        }
        // player.camera.camTarget.x = 0.f;
        // player.camera.camTarget.y = 0.f;
        // player.camera.camtarget.z = 1.f;
        // std::cout << "chair" << std::endl;
        if (player.pEntity.velocity > 11.5f && gData.hasChairScared == false && gData.hasAudioLevel1 == true) {
            PauseSound(chairSound);
            PlaySound(chairScared);
            // chairEntity.location.y = 25.f;
            applyForce({250.f,250.f,0.f}, chairEntity.pEntity);
            gData.hasChairScared = true;
        }
    }
}

void updateRoombaToViewport(meshedObject& roomba, player& player, float deltaTime) {
    float yaw   = player.camera.camTarget.x;
    float pitch = player.camera.camTarget.y;

    vector3 forward = {
        cosf(pitch) * cosf(yaw),
        sinf(pitch),
        cosf(pitch) * sinf(yaw)
    };
    vector3 right = normalize3({ sinf(yaw), 0.f, -cosf(yaw) });
    vector3 up    = normalize3(cross3(right, forward));

    float dist   = 2.f;
    float halfH  = dist * tanf(player.camera.fov * 0.5f);
    float halfW  = halfH * player.camera.aspect;

    vector3 target = player.camera.camPos
        + forward.fmult(dist)
        - right.fmult(halfW * 0.75f)
        - up.fmult(halfH * 0.75f)
        - vector3{0.f, 1.f, 0.f};

    float speed = 8.f;
    roomba.pEntity.location.x += (target.x - roomba.pEntity.location.x) * speed * deltaTime;
    roomba.pEntity.location.y += (target.y - roomba.pEntity.location.y) * speed * deltaTime;
    roomba.pEntity.location.z += (target.z - roomba.pEntity.location.z) * speed * deltaTime;

    vector3 toCamera = player.camera.camPos - roomba.pEntity.location;
    roomba.rotY = -atan2f(toCamera.x, toCamera.z);
}

void levelLogic(player& player, float deltaTime,gameData& gData, Sound deathSound, Sound level1win, Sound level2win, Sound level3win, Sound bgMusicLevel1, Sound bgMusicLevel2, Sound bgMusicLevel3, Sound chairSound, Sound chairScared, Sound Roomba, meshedObject& chairEntity, meshedObject& roomba) {
    // std::cout << "current plane: " << player.pEntity.groundPlane << std::endl;
    if (gData.lives <= 0) {
        gData.gameEnded = true;
        gData.currentLevel = gameData::GAMEEND;
    }
    if (gData.currentLevel == gameData::LEVEL1) {
        if (!IsSoundPlaying(bgMusicLevel1)) {
            PlaySound(bgMusicLevel1);
        }
        if (IsSoundPlaying(bgMusicLevel2)) {
            StopSound(bgMusicLevel2);
        }
        if(IsSoundPlaying(bgMusicLevel3)) {
            StopSound(bgMusicLevel3);
        }
        checkChair(player, gData, chairSound, chairScared, chairEntity);
        updateEntityLocation(chairEntity);
        updateColliderLocation(chairEntity);
        if (player.pEntity.groundPlane == 4 && gData.bgMusicLevel1 == false) {
            gData.bgMusicLevel1 = true;
            std::thread([&player, &gData, bgMusicLevel1, level1win]() {
                auto start = std::chrono::steady_clock::now();
                while (true) {
                    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
                    gData.fadeTo = 1.f - std::clamp(elapsed, 0.f, 1.f);
                    SetSoundVolume(bgMusicLevel1, 1.f - std::clamp(elapsed, 0.f, 1.f));
                    if (elapsed >= 1.f) break;
                }
                // PlaySound(level1win);
                StopSound(bgMusicLevel1);
                gData.currentLevel = gameData::LEVEL2;
                player.pEntity.location = {0.f, 5.f, 0.f};
                std::this_thread::sleep_for(std::chrono::seconds(1));

                start = std::chrono::steady_clock::now();
                while (true) {
                    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
                    gData.fadeTo = std::clamp(elapsed, 0.f, 1.f);
                    if (elapsed >= 1.f) break;
                }
            }).detach();
        }
    }
    if (gData.currentLevel == gameData::LEVEL2) {
        if (!IsSoundPlaying(bgMusicLevel2)) {
            PlaySound(bgMusicLevel2);
        }
        if (IsSoundPlaying(bgMusicLevel1)) {
            StopSound(bgMusicLevel1);
        }
        if(IsSoundPlaying(bgMusicLevel3)) {
            StopSound(bgMusicLevel3);
        }
        // if (gData.currentLevel == gameData::LEVEL2)
        if (gData.hasAudioLevel2 == false) {
            gData.hasAudioLevel2 = true;
            PlaySound(Roomba);
        }
        updateRoombaToViewport(roomba, player, deltaTime);
        updateEntityLocation(roomba);
        if (player.pEntity.groundPlane == 245 && gData.bgMusicLevel2 == false) {
            gData.bgMusicLevel2 = true;
                std::thread([&player, &gData, bgMusicLevel2, level2win]() {
                auto start = std::chrono::steady_clock::now();
                while (true) {
                    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
                    gData.fadeTo = 1.f - std::clamp(elapsed, 0.f, 1.f);
                    SetSoundVolume(bgMusicLevel2, 1.f - std::clamp(elapsed, 0.f, 1.f));
                    if (elapsed >= 1.f) break;
                }
                StopSound(bgMusicLevel2);
                PlaySound(level2win);
                gData.currentLevel = gameData::LEVEL3;
                player.pEntity.location = {0.f, 5.f, 0.f};
                std::this_thread::sleep_for(std::chrono::seconds(3));

                start = std::chrono::steady_clock::now();
                while (true) {
                    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
                    gData.fadeTo = std::clamp(elapsed, 0.f, 1.f);
                    if (elapsed >= 1.f) break;
                }
            }).detach();
        }
    }
    if (gData.currentLevel == gameData::LEVEL3 && gData.bgMusicLevel3 == false) {
        if (!IsSoundPlaying(bgMusicLevel3)) {
            PlaySound(bgMusicLevel3);
        }
        if (IsSoundPlaying(bgMusicLevel2)) {
            StopSound(bgMusicLevel2);
        }
        if(IsSoundPlaying(bgMusicLevel1)) {
            StopSound(bgMusicLevel1);
        }
        if (player.pEntity.groundPlane == 9 && gData.bgMusicLevel3 == false) {
            gData.bgMusicLevel3 = true;
                std::thread([&player, &gData, bgMusicLevel3, level3win]() {
                auto start = std::chrono::steady_clock::now();
                while (true) {
                    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
                    gData.fadeTo = 1.f - std::clamp(elapsed, 0.f, 1.f);
                    SetSoundVolume(bgMusicLevel3, 1.2f - std::clamp(elapsed, 0.f, 1.f));
                    if (elapsed >= 1.f) break;
                }
                // StopSound(bgMusicLevel3);
                PlaySound(level3win);
                gData.currentLevel = gameData::GAMEWIN;
                player.pEntity.location = {0.f, 5.f, 0.f};
                std::this_thread::sleep_for(std::chrono::seconds(3));

                start = std::chrono::steady_clock::now();
                while (true) {
                    float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
                    gData.fadeTo = std::clamp(elapsed, 0.f, 1.f);
                    if (elapsed >= 1.f) break;
                }
            }).detach();
        }
    }
    if (player.pEntity.location.y < -10.f) {
        if (!gData.isDying) {
            PlaySound(deathSound);
            gData.isDying = true;
        }        
        if (player.pEntity.location.y < -50.f) {
            float t = (player.pEntity.location.y + 50.f) / (-150.f + 50.f);
            gData.fadeTo = std::clamp(1.f - t, 0.f, 1.f);

            if (player.pEntity.location.y < -150.f) {
                gData.fadeTo = 0.f;
                player.pEntity.location = {0.f, 5.f, 0.f};
                player.pEntity.magnitude = {0.f, 0.f, 0.f};
                gData.lives--;
                gData.fadeTo = 1.f;
                gData.isDying = false;
                chairEntity.pEntity.magnitude = {0.f, 0.f, 0.f};
                chairEntity.pEntity.acceleration = {0.f, 0.f, 0.f};
                chairEntity.pEntity.location = {0.f, 0.f, 0.f};
                resetMeshedLocation(chairEntity);
                gData.hasChairScared = false;
            }
        }
    }
}
