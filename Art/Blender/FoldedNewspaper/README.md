# FoldedNewspaperPrototype

UE-ready blank folded-newspaper prototype for RulesHorror.

## Files

- `FoldedNewspaperPrototype.blend`: clean Blender source. Scene contains only:
  - `SK_Newspaper_3Sheets` mesh
  - `ARM_Newspaper_3Sheets` armature
- `Exports/SK_Newspaper_3Sheets.fbx`: Unreal import FBX.
- `Textures/T_NewspaperPaper_Aged_BaseColor.png`: aged blank newspaper paper texture.
- `create_folded_newspaper_blend.py`: regeneration/export script.

## Geometry

- 3 newspaper sheets.
- Full open size: `0.60m x 0.84m`.
- Coordinate plane: horizontal XY paper plane.
  - X = width / short side `0.60m`
  - Y = height / long side `0.84m`
  - Z = paper thickness / stacked sheet offset
- No readable printed newspaper content is included; add content in Unreal with WidgetComponent/material layering.
- The base color texture is intentionally newsprint-like: gray recycled paper, fibers, pulp speckles, faint ink transfer/halftone residue, edge/fold wear.

## Correct fold requirement

The first fold must halve the long side:

- Full open: `0.60 x 0.84`
- First fold: `0.60 x 0.42`
- Second fold: `0.30 x 0.42`

## Default state

The source `.blend` and exported `.fbx` now use a two-step folded idle action as the active/default action:

- `AN_Default_FoldedPose`
  - Frame `1`: folded twice `0.30 x 0.42`
  - Frame `2`: folded twice `0.30 x 0.42`

In Unreal, use this animation as the SkeletalMeshComponent default animation if the placed newspaper should appear folded before interaction.

## Rig hierarchy

The second fold was rebuilt to avoid twisting:

- `*_Root_TR`: fixed top-right reference quadrant.
- `*_Fold_H_Bottom_Right`: bottom-right panel, folds the long `0.84` side first.
- `*_Fold_V_Left_Group`: whole left-column group, folds second.
- `*_Fold_H_Bottom_Left`: bottom-left panel, child of `*_Fold_V_Left_Group`.

This means after the first fold, the already folded left column rotates as one group during the second fold.

## Widget attachment helper bones

The FBX includes non-deforming helper bones that can be used as WidgetComponent attachment targets.
They are exported as bones, so in Unreal you can attach directly to the bone name or create Skeleton sockets on these bones for editor-friendly offsets.

Per sheet:

- `Sheet_##_Socket_Widget_TR`: top-right quarter panel, parented to `Sheet_##_Root_TR`
- `Sheet_##_Socket_Widget_BR`: bottom-right quarter panel, parented to `Sheet_##_Fold_H_Bottom_Right`
- `Sheet_##_Socket_Widget_TL`: top-left quarter panel, parented to `Sheet_##_Fold_V_Left_Group`
- `Sheet_##_Socket_Widget_BL`: bottom-left quarter panel, parented to `Sheet_##_Fold_H_Bottom_Left`

Use separate WidgetComponents per quarter panel if the UI must follow the folding paper.
A single large WidgetComponent will stay planar and will not deform across fold lines.

## Folding clips: full open -> folded

- `AN_Fold_01_FullOpenToWideHalfFolded`
  - folds the long `0.84` side to `0.42`
  - verified dimensions: `X 0.60 / Y 0.84` -> `X 0.60 / Y 0.42`
  - frames `1-60`
- `AN_Fold_02_WideHalfFoldedToFolded`
  - folds the left-column group from `0.60` width to `0.30`
  - verified dimensions: `X 0.60 / Y 0.42` -> `X 0.30 / Y 0.42`
  - frames `1-60`

Preview action:

- `AN_Preview_FullTwoStepFold`
  - Frame `1`: full open `0.60 x 0.84`
  - Frame `60`: first fold complete `0.60 x 0.42`
  - Frame `120`: second fold complete `0.30 x 0.42`

## Unfolding clips: folded -> full open

- `AN_Unfold_01_FoldedToWideHalfOpen`
  - folded twice -> wide half-open `0.60 x 0.42`
  - frames `1-60`
- `AN_Unfold_02_WideHalfOpenToFullOpen`
  - wide half-open -> full open `0.60 x 0.84`
  - frames `1-60`

Preview action:

- `AN_Preview_FullTwoStepUnfold`
  - Frame `1`: folded twice `0.30 x 0.42`
  - Frame `60`: wide half-open `0.60 x 0.42`
  - Frame `120`: full open `0.60 x 0.84`

## Import notes

- Import `Exports/SK_Newspaper_3Sheets.fbx` as a Skeletal Mesh in Unreal.
- Import animations from the same FBX for the same skeleton.
- For the initial closed newspaper state, set the component's default animation to `AN_Default_FoldedPose`.

