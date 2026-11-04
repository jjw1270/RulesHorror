# Builds /Game/Objects/Materials/M_CRTScreen and MI_CRTScreen_Office in the running editor.
# Run through the Unreal MCP server: ProgrammaticToolset.execute_tool_script with this file as `script`
# (e.g. Saved/ClaudeTmp/pipeline/ue_call.py script Art/Materials/CRTScreen/build_m_crtscreen.py).
# The Custom node body comes from crt_screen.hlsl next to this file: copy it to Saved/ClaudeTmp/crt_screen_hlsl.txt first.
import json

MT = 'editor_toolset.toolsets.material.MaterialTools.'
MI = 'editor_toolset.toolsets.material_instance.MaterialInstanceTools.'
O = 'editor_toolset.toolsets.object.ObjectTools.'
AT = 'editor_toolset.toolsets.asset.AssetTools.'
HLSL = 'F:/UnrealProjects/RulesHorror/Saved/ClaudeTmp/crt_screen_hlsl.txt'  # copy of crt_screen.hlsl (MCP read_file only allows .txt etc. under Content/ and Saved/)
FOLDER = '/Game/Objects/Materials'
MAT = FOLDER + '/M_CRTScreen'
INST = FOLDER + '/MI_CRTScreen_Office'

# (custom input name, parameter name, default)
SCALARS = [
    ('FilterOn', 'Filter On', 0.0),          # set to 0/1 by AComputer::SetPower
    ('Intensity', 'Emissive Intensity', 1.0),
    ('ScanCount', 'Scanline Count', 300.0),
    ('ScanInt', 'Scanline Intensity', 0.4),
    ('MaskInt', 'Mask Intensity', 0.3),
    ('MaskScale', 'Mask Scale', 160.0),
    ('Curv', 'Curvature', 0.02),
    ('Vig', 'Vignette', 0.35),
    ('Flick', 'Flicker', 0.02),
    ('Noise', 'Noise', 0.03),
    ('RollInt', 'Roll Bar Intensity', 0.06),
    ('RollSpeed', 'Roll Bar Speed', 0.12),
    ('Glow', 'Glow', 0.35),
]


def t(n, a):
    return execute_tool(n, json.dumps(a))['returnValue']


def obj(p):
    return {'refPath': p + '.' + p.split('/')[-1]}


def add(m, cls, x, y, props=None):
    e = t(MT + 'add_expression', {'material_or_function': m, 'expression_class': {'refPath': '/Script/Engine.' + cls}, 'x': x, 'y': y})
    if props:
        if not t(O + 'set_properties', {'instance': e, 'values': json.dumps(props)}):
            raise RuntimeError('set_properties failed on ' + cls)
    return e


def run():
    if not t(AT + 'exists', {'path': MAT}):
        t(MT + 'create_material', {'folder_path': FOLDER, 'asset_name': 'M_CRTScreen'})
    m = obj(MAT)
    for e in t(MT + 'get_expressions', {'material_or_function': m}):
        t(MT + 'delete_expression', {'material_or_function': m, 'expression': e})

    code = t(AT + 'read_file', {'file_path': HLSL})
    inputs = [{'inputName': 'UV'}, {'inputName': 'Time'}, {'inputName': 'Tex'}] + [{'inputName': s[0]} for s in SCALARS]
    custom = add(m, 'MaterialExpressionCustom', 0, 0, {'code': code, 'outputType': 'CMOT_Float3', 'description': 'CRT'})
    # set_properties cannot rename and resize an array in one call: rename the default entry, then grow one by one
    for n in range(len(inputs)):
        cur = json.loads(t(O + 'get_properties', {'instance': custom, 'properties': ['inputs']}))['inputs']
        if n == 0:
            cur[0]['inputName'] = inputs[0]['inputName']
        else:
            cur.append({'inputName': inputs[n]['inputName'], 'input': json.loads(json.dumps(cur[0]['input']))})
        if not t(O + 'set_properties', {'instance': custom, 'values': json.dumps({'inputs': cur})}):
            raise RuntimeError('custom inputs failed at %d' % n)

    uv = add(m, 'MaterialExpressionTextureCoordinate', -500, -300)
    tm = add(m, 'MaterialExpressionTime', -500, -200)
    tex = add(m, 'MaterialExpressionTextureObjectParameter', -500, -100,
              {'parameterName': 'Display Input', 'texture': {'refPath': '/Engine/EngineResources/Black.Black'}, 'group': 'Screen'})
    t(MT + 'connect_expressions', {'from_expression': uv, 'from_output_name': '', 'to_expression': custom, 'to_input_name': 'UV'})
    t(MT + 'connect_expressions', {'from_expression': tm, 'from_output_name': '', 'to_expression': custom, 'to_input_name': 'Time'})
    t(MT + 'connect_expressions', {'from_expression': tex, 'from_output_name': '', 'to_expression': custom, 'to_input_name': 'Tex'})
    for i, (pin, name, dv) in enumerate(SCALARS):
        sp = add(m, 'MaterialExpressionScalarParameter', -500, 20 + i * 70,
                 {'parameterName': name, 'defaultValue': dv, 'group': 'Screen' if i < 2 else 'CRT', 'sortPriority': i})
        t(MT + 'connect_expressions', {'from_expression': sp, 'from_output_name': '', 'to_expression': custom, 'to_input_name': pin})
    t(MT + 'connect_to_output', {'expression': custom, 'output_name': '', 'material_property': 'MP_EmissiveColor'})

    glass = add(m, 'MaterialExpressionVectorParameter', -250, -500,
                {'parameterName': 'Glass Color', 'defaultValue': {'r': 0.012, 'g': 0.015, 'b': 0.014, 'a': 1.0}, 'group': 'Glass'})
    rough = add(m, 'MaterialExpressionScalarParameter', -250, -380, {'parameterName': 'Glass Roughness', 'defaultValue': 0.12, 'group': 'Glass'})
    spec = add(m, 'MaterialExpressionScalarParameter', -250, -300, {'parameterName': 'Glass Specular', 'defaultValue': 0.5, 'group': 'Glass'})
    t(MT + 'connect_to_output', {'expression': glass, 'output_name': '', 'material_property': 'MP_BaseColor'})
    t(MT + 'connect_to_output', {'expression': rough, 'output_name': '', 'material_property': 'MP_Roughness'})
    t(MT + 'connect_to_output', {'expression': spec, 'output_name': '', 'material_property': 'MP_Specular'})

    t(MT + 'layout_expressions', {'material_or_function': m})
    t(MT + 'recompile', {'material_or_function': m})
    if not t(AT + 'exists', {'path': INST}):
        t(MI + 'create', {'folder_path': FOLDER, 'asset_name': 'MI_CRTScreen_Office', 'parent': m})
    params = t(MI + 'list_parameters', {'material': obj(INST)})
    t(AT + 'save_assets', {'asset_paths': [MAT, INST]})
    return {'params': params}
