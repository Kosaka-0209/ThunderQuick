#include <Siv3D.hpp>
#include "Game.hpp"

void Main()
{
    Window::Resize(1280, 720);
    Window::SetTitle(U"LINE / BLADE - PLAN & DODGE");
    Scene::SetBackground(ColorF{ 0.025, 0.045, 0.08 });
    Game game;

    // 毎フレーム、「入力と更新」→「描画」の順に実行する。
    while (System::Update())
    {
        game.update();
        game.draw();
    }
}
