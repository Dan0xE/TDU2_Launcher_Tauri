<script setup lang="ts">
import { Ref, ref } from "vue";
import { invoke } from "@tauri-apps/api/tauri";
import { open } from "@tauri-apps/api/dialog";

let game_path: Ref<string> = ref("");
let is_suspended: Ref<boolean> = ref(false);
let game_is_running: Ref<boolean> = ref(false)
const err_message: Ref<string>= ref("")

function sleep(milliseconds: number) : Promise<unknown>{
  return new Promise((resolve): number => setTimeout(resolve, milliseconds))
}

async function handle_game_thread(): Promise<void> {
    invoke("handle_thread").then((res) => {
      res ? err_message.value = "Sucessfully suspended/resumed thread" : err_message.value = "Error while suspending/resuming thread"
    })
    is_suspended.value = !is_suspended.value
}

async function select_game_path(): Promise<void>
 {
    open({directory: true}).then((res: string | string[] | null) : string => res ? game_path.value = res as string : err_message.value = "No Path Set" )
}

async function check_if_game_is_running() : Promise<void>{
    invoke("is_game_running").then((res: unknown): boolean => game_is_running.value = res as unknown as boolean)
    while(true) {
      await sleep(2000)
      invoke("is_game_running").then((res: unknown): boolean => game_is_running.value = res as unknown as boolean)
    }
}

check_if_game_is_running()

async function inject() : Promise<void>{
  game_is_running.value ?
  open().then((res: string | string[] | null) : void=> {
    res ? invoke('inject', {path : res}).then((res: unknown  ): "DLL Injected" | "Failed to inject DLL" => res ? err_message.value = "DLL Injected" : err_message.value = "Failed to inject DLL" ) : err_message.value = "DLL not selected"
  }) : err_message.value = "Game is not running, start the game before injecting"
}

async function launch_game () : Promise<void>{
  game_path.value ?
  invoke("run_game", { path: game_path.value }).then((res: unknown): string => err_message.value = res as string)
  : err_message.value = "game path not set"
}
</script>

<template>
    <button v-on:click="select_game_path()">Select Game Path</button>
    <button v-on:click="inject()">Inject DLL</button>
    <button v-if="game_is_running" v-on:click="handle_game_thread()">{{ is_suspended ? "Resume Thread" : "Suspend Thread" }}</button>
    <button v-if="!game_is_running" v-on:click="launch_game()">Launch Game</button>

  <p>{{ game_is_running ? "Game is running (close the game before launching)" : "Game is not running" }}</p>
  <p>{{ err_message }}</p>
</template>
