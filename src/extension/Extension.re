type state =
  | NotConfigured
  | Disabled(string)
  | Active(Config.t);

let state = ref(NotConfigured);

let load = pi =>
  switch (Config.decode(Js.Dict.get(Pi.getSettings(pi), Settings.key))) {
  | Ok(None) => NotConfigured
  | Ok(Some(config)) => Active(config)
  | Error(message) => Disabled(message)
  };

let activate = pi =>
  Pi.onSessionStart(
    pi,
    (_event, ctx) => {
      state := load(pi);
      switch (state^) {
      | Disabled(message) =>
        Pi.notify(
          ctx.ui,
          Printf.sprintf(
            "%s is disabled, fix its configuration: %s",
            Settings.key,
            message,
          ),
          `error,
        )
      | NotConfigured
      | Active(_) => ()
      };
    },
  );
