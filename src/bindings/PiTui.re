type theme;

type component;

type color = [
  | `toolTitle
  | `accent
  | `muted
  | `dim
];

[@mel.send] external fg: (theme, color, string) => string = "fg";

[@mel.send] external bold: (theme, string) => string = "bold";

[@mel.new] [@mel.module "@earendil-works/pi-tui"]
external makeText: (string, int, int) => component = "Text";

let text = content => makeText(content, 0, 0);
