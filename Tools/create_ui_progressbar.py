# -*- coding: utf-8 -*-
"""
열차 진행도 ProgressBar 스킨 — 스팀펑크/JRPG 톤 UI 머테리얼 2종 생성

표준 UProgressBar 의 Style 브러시 2개를 교체하는 방식이라 C++ 수정이 필요 없다
(PB_TrainProgress->SetPercent(Progress) 를 그대로 둔다. 채움 클리핑은 위젯이 처리).

  MI_ProgressBar_BG   → Style.Background 브러시 (속 트랙: 어두운 함몰 배경만)
  MI_ProgressBar_Fill → Style.Fill 브러시       (채움: 툰 플랫 — 셀 2톤 + 식은 꼬리
                                                 + 평평한 대각 라인. 발광/글로우 없음)

프레임(강철 틀/기어/뱃지)은 유저가 만든 PNG 텍스처로 넣는다. 절차적 머테리얼은 속 트랙 + 불꽃만.

실행(에디터 Output Log 콘솔, Cmd→Python 모드):
  py "C:/Users/start/Documents/Unreal Projects/Team4Project/Tools/create_ui_progressbar.py"

UMG 연결 — 프레임이 채움에 안 가려지고 항상 뜨게 하려면 Overlay 3레이어:
  [Overlay]
   ├─ (하) ProgressBar PB_TrainProgress   ← Background=MI_ProgressBar_BG, Fill=MI_ProgressBar_Fill (Draw As=Box)
   ├─ (중) Image  Train_img               ← 기존 기관차, 진행도 따라 이동
   └─ (상) Image  Img_BarFrame            ← 유저 제작 프레임 PNG(가운데 투명), Visibility=HitTestInvisible
 * 프레임 Image 를 맨 위에 두므로 채움이 차올라도 절대 안 사라진다.
 * ProgressBar Bar Fill Type = Left To Right, 채움 비율은 기존 SetPercent(Progress) 그대로.
 * 프레임 PNG 가 자체 배경(투명 아님)을 포함하면 Background 브러시는 None 으로 둬도 된다.
"""

import unreal

PKG_PATH = "/Game/GameSystem/UI"

# 예전 단일 머테리얼(있으면 정리) — 이 스크립트가 BG/Fill 2종으로 대체
LEGACY = ["M_ProgressBar", "MI_ProgressBar"]


# ─────────────────────────────────────────────────────────────────────────────
# 빈 바 (Background): 함몰된 어두운 속 트랙만 담당.
#   프레임(강철 틀/기어/뱃지)은 유저가 만든 PNG 를 위에 오버레이하므로 여기선 안 그린다.
#   (프레임과 겹치지 않게 안쪽 트랙 배경만 → 유저 프레임의 투명 창 뒤로 깔린다)
# ─────────────────────────────────────────────────────────────────────────────
BG_HLSL = r"""
float2 uv = UV.xy;
float2 p  = uv * float2(BarAspect, 1.0);
float2 hs = float2(BarAspect, 1.0) * 0.5;

// ── 알약형 트랙 마스크 ──
float2 dd = abs(p - hs) - (hs - CornerRadius);
float dist = length(max(dd, 0.0)) + min(max(dd.x, dd.y), 0.0) - CornerRadius;
float aa = fwidth(dist);
float mask = 1.0 - smoothstep(0.0, aa, dist);

// ── 함몰 트랙: 상단 내부 그림자 → 하단 약간 밝게 ──
float shade = smoothstep(0.0, 0.55, uv.y);
float3 col = lerp(TrackTop.rgb, TrackBottom.rgb, shade);

// ── 안쪽 가장자리 림(홈 입체) ──
float inner = 1.0 - smoothstep(-InnerEdge, -InnerEdge + aa, dist);
float rim = saturate(mask - inner);
col = lerp(col, TrackRim.rgb, rim);

return float4(col, mask);
"""

BG_SCALARS = [
    ("BarAspect",    12.0),   # 바 가로/세로 비 — 실제 바 비율에 맞춰 조절
    ("CornerRadius", 0.5),    # 모서리 둥글기 (0.5=완전 알약형)
    ("InnerEdge",    0.06),   # 안쪽 가장자리 림 두께
]
BG_VECTORS = [
    ("TrackTop",    (0.05, 0.05, 0.07, 1.0)),  # 트랙 상단(어두운 내부 그림자)
    ("TrackBottom", (0.10, 0.10, 0.13, 1.0)),  # 트랙 하단(살짝 밝음)
    ("TrackRim",    (0.18, 0.19, 0.24, 1.0)),  # 안쪽 가장자리 림
]


# ─────────────────────────────────────────────────────────────────────────────
# 채움 (Fill): 툰 플랫 채움 — 발광(글로우) 없음.
#   셀 2톤(상단 하이라이트 밴드 + 본색, 하드 경계) + 뒤쪽 살짝 식은 톤 + 평평한 대각 라인.
#   위젯이 진행률만큼 가로로 잘라 그리므로 UV.x=1 이 곧 채움 선두 에지가 된다.
# ─────────────────────────────────────────────────────────────────────────────
FILL_HLSL = r"""
float2 uv = UV.xy;
float t = Time;

// ── 알약형 SDF ──
float2 p  = uv * float2(FillAspect, 1.0);
float2 hs = float2(FillAspect, 1.0) * 0.5;
float2 dd = abs(p - hs) - (hs - CornerRadius);
float dist = length(max(dd, 0.0)) + min(max(dd.x, dd.y), 0.0) - CornerRadius;
float aa = fwidth(dist);
float inMask = 1.0 - smoothstep(0.0, aa, dist);

// ── 툰 2톤: 상단 하이라이트 밴드 + 하단 본색 (하드 경계) ──
float3 col = lerp(FillColor.rgb, FillColorTop.rgb, step(uv.y, CelSplit));

// ── 진행 방향감: 뒤쪽(꼬리) 살짝 식은 톤 (하드 스텝, 글로우 아님) ──
col = lerp(col, col * TailTint.rgb, step(uv.x, TailSplit));

// ── 대각선 세그먼트 라인 (플랫 톤 오버레이, additive 아님) ──
//    SegSkew=기울기 · SegDashGap=점선/실선 · SegScroll=흐름속도(0=정지)
float diag = uv.x * SegCount + uv.y * SegSkew;
float stripe = frac(diag - t * SegScroll);
float lineMask = smoothstep(1.0 - SegThickness, 1.0, stripe);
float dash = step(SegDashGap, frac(uv.y * SegDashCount));
col = lerp(col, LineColor.rgb, lineMask * dash * SegStrength);

// ── 하단 플랫 그림자(툰 접지) ──
col = lerp(col, col * BottomShade, step(BottomSplit, uv.y));

return float4(col, inMask);
"""

FILL_SCALARS = [
    ("FillAspect",   12.0),
    ("CornerRadius", 0.5),
    ("CelSplit",     0.4),    # 상단 하이라이트 밴드 경계 (이 위쪽이 밝음)
    ("TailSplit",    0.22),   # 꼬리(식은 톤) 비율 (0=끔)
    ("SegCount",     14.0),   # 대각선 라인 개수(가로 방향)
    ("SegSkew",      3.0),    # 대각 기울기 (0=세로선, 클수록 눕는다)
    ("SegScroll",    0.0),    # 라인 흐름 속도 (0=정지=차분한 툰)
    ("SegThickness", 0.12),   # 라인 굵기
    ("SegDashGap",   0.35),   # 점선 간격 (0=실선, 클수록 끊김↑)
    ("SegDashCount", 6.0),    # 세로 점선 분할 수
    ("SegStrength",  0.22),   # 라인 진하기 (0=끔)
    ("BottomSplit",  0.72),   # 하단 그림자 시작 높이
    ("BottomShade",  0.82),   # 하단 그림자 어둡기 (1=없음)
]
FILL_VECTORS = [
    ("FillColor",    (0.82, 0.40, 0.14, 1.0)),  # 채움 본색 — 석탄 앰버(플랫)
    ("FillColorTop", (0.98, 0.62, 0.26, 1.0)),  # 상단 하이라이트 밴드
    ("TailTint",     (0.72, 0.62, 0.60, 1.0)),  # 꼬리 식은 톤 배수(어둡게)
    ("LineColor",    (0.98, 0.70, 0.34, 1.0)),  # 대각 라인(살짝 밝은 플랫)
]


def build_ui_material(name, hlsl, scalars, vectors, use_time):
    """UI 도메인 반투명 머테리얼 + 동명 MI(MI_<name 뒤>) 생성."""
    mat_path = PKG_PATH + "/M_" + name
    mi_path = PKG_PATH + "/MI_" + name
    for p in (mi_path, mat_path):
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            unreal.EditorAssetLibrary.delete_asset(p)

    at = unreal.AssetToolsHelpers.get_asset_tools()
    mat = at.create_asset("M_" + name, PKG_PATH, unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        unreal.log_error("머티리얼 생성 실패: " + name)
        return

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    mel = unreal.MaterialEditingLibrary

    custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("description", name + " (toon, steampunk dusk palette)")
    custom.set_editor_property("code", hlsl)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)

    input_names = ["UV"]
    if use_time:
        input_names.append("Time")
    input_names += [n for n, _ in scalars] + [n for n, _ in vectors]

    inputs = []
    for nm in input_names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nm)
        inputs.append(ci)
    custom.set_editor_property("inputs", inputs)

    texco = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -700, -260)
    texco.set_editor_property("coordinate_index", 0)
    mel.connect_material_expressions(texco, "", custom, "UV")

    if use_time:
        tnode = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -700, -180)
        mel.connect_material_expressions(tnode, "", custom, "Time")

    y = -600
    for nm, default in scalars:
        pnode = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, y)
        pnode.set_editor_property("parameter_name", nm)
        pnode.set_editor_property("default_value", float(default))
        mel.connect_material_expressions(pnode, "", custom, nm)
        y += 70

    for nm, (r, g, b, a) in vectors:
        pnode = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -700, y)
        pnode.set_editor_property("parameter_name", nm)
        pnode.set_editor_property("default_value", unreal.LinearColor(r, g, b, a))
        mel.connect_material_expressions(pnode, "", custom, nm)
        y += 110

    # float4 → RGB(Final Color) + A(Opacity)
    mask_rgb = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 0, -50)
    mask_rgb.set_editor_property("r", True)
    mask_rgb.set_editor_property("g", True)
    mask_rgb.set_editor_property("b", True)
    mask_rgb.set_editor_property("a", False)
    mel.connect_material_expressions(custom, "", mask_rgb, "")

    mask_a = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 0, 100)
    mask_a.set_editor_property("r", False)
    mask_a.set_editor_property("g", False)
    mask_a.set_editor_property("b", False)
    mask_a.set_editor_property("a", True)
    mel.connect_material_expressions(custom, "", mask_a, "")

    mel.connect_material_property(mask_rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(mask_a, "", unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)

    mi = at.create_asset("MI_" + name, PKG_PATH, unreal.MaterialInstanceConstant,
                         unreal.MaterialInstanceConstantFactoryNew())
    if mi:
        mel.set_material_instance_parent(mi, mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mi)

    unreal.log("생성 완료: {} + {}".format(mat_path, mi_path))


def main():
    # 예전 단일 머테리얼 정리
    for nm in LEGACY:
        p = PKG_PATH + "/" + nm
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            unreal.EditorAssetLibrary.delete_asset(p)
            unreal.log("예전 에셋 삭제: " + p)

    build_ui_material("ProgressBar_BG", BG_HLSL, BG_SCALARS, BG_VECTORS, use_time=False)
    build_ui_material("ProgressBar_Fill", FILL_HLSL, FILL_SCALARS, FILL_VECTORS, use_time=True)

    unreal.log("→ PB_TrainProgress 의 Style: Background=MI_ProgressBar_BG, Fill=MI_ProgressBar_Fill (Draw As=Box)")


main()
