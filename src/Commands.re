let compileRescript = (~compileCommand: string, ~logger: Log.logger) => {
  let durationLabel = "[Commands.compileRescript] duration";
  Js.Console.timeStart(durationLabel);

  logger.info(() =>
    Js.log("[Commands.compileRescript] Compiling fresh React app files...")
  );

  switch (
    ChildProcess.spawnSync(
      compileCommand,
      [||],
      {"shell": true, "encoding": "utf8", "stdio": "inherit"},
    )
  ) {
  | Ok () =>
    logger.info(() => {Js.log("[Commands.compileRescript] Success!")});
    Js.Console.timeEnd(durationLabel);
  | Error(JsError(error)) =>
    logger.info(() => {
      Js.Console.error2("[Commands.compileRescript] Failure! Error:", error)
    });
    Process.exit(1);
  | Error(ExitCodeIsNotZero(exitCode)) =>
    Js.Console.error2(
      "[Commands.compileRescript] Failure! Exit code is not zero:",
      exitCode,
    );
    Process.exit(1);
  | exception (Js.Exn.Error(error)) =>
    logger.info(() => {
      Js.Console.error2(
        "[Commands.compileRescript] Exception:\n",
        error->Js.Exn.message,
      )
    });
    Process.exit(1);
  };
};

let buildPagesWithWorkers = (~pages, ~outputDir, ~logger, ~globalValues) =>
  pages
  ->Array.splitIntoChunks(~chunkSize=1)
  ->Js.Array2.map((pages, ()) =>
      pages
      ->Js.Array2.map(page =>
          RebuildPageWorkerHelpers.rebuildPagesWithWorker(
            ~outputDir,
            ~logger,
            ~globalValues,
            [|page|],
          )
        )
      ->Js.Promise.all
      ->Promise.map(result => Array.flat1(result))
    )
  ->Promise.seqRun
  ->Promise.map(result => Array.flat1(result));

let build =
    (
      ~outputDir: string,
      ~compileCommand: string,
      ~logLevel: Log.level,
      ~mode: Webpack.Mode.t,
      ~pages: array(PageBuilder.page),
      ~webpackBundleAnalyzerMode=None,
      ~minimizer: Webpack.Minimizer.t=Terser,
      ~globalValues: array((string, string))=[||],
      (),
    ) => {
  let logger = Log.makeLogger(logLevel);

  let webpackPages =
    buildPagesWithWorkers(~pages, ~outputDir, ~logger, ~globalValues);

  webpackPages
  ->Promise.map(webpackPages => {
      let () = compileRescript(~compileCommand, ~logger);
      let () =
        Webpack.build(
          ~mode,
          ~outputDir,
          ~logger,
          ~webpackBundleAnalyzerMode,
          ~minimizer,
          ~globalValues,
          ~webpackPages,
        );
      ();
    })
  ->ignore;
};

let start =
    (
      ~outputDir: string,
      ~mode: Webpack.Mode.t,
      ~logLevel: Log.level,
      ~pages: array(PageBuilder.page),
      ~devServerOptions: Webpack.DevServerOptions.t,
      ~webpackBundleAnalyzerMode:
         option(Webpack.WebpackBundleAnalyzerPlugin.Mode.t),
      ~minimizer: Webpack.Minimizer.t=Terser,
      ~globalValues: array((string, string))=[||],
      (),
    ) => {
  let () = GlobalValues.unsafeAdd(globalValues);

  let logger = Log.makeLogger(logLevel);

  let webpackPages =
    buildPagesWithWorkers(~pages, ~outputDir, ~logger, ~globalValues);

  webpackPages
  ->Promise.map(webpackPages => {
      let () =
        Webpack.startDevServer(
          ~devServerOptions,
          ~webpackBundleAnalyzerMode,
          ~mode,
          ~logger,
          ~outputDir,
          ~minimizer,
          ~globalValues,
          ~webpackPages,
        );
      ();
    })
  ->ignore;

  let () = Watcher.startWatcher(~outputDir, ~logger, ~globalValues, pages);

  ();
};
