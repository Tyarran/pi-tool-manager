type schema;

[@mel.module "typebox"] [@mel.scope "Type"]
external string_: Js.t({..}) => schema = "String";

[@mel.module "typebox"] [@mel.scope "Type"]
external integer: Js.t({..}) => schema = "Integer";

[@mel.module "typebox"] [@mel.scope "Type"]
external optional: schema => schema = "Optional";

[@mel.module "typebox"] [@mel.scope "Type"]
external object_: (Js.Dict.t(schema), Js.t({..})) => schema = "Object";
