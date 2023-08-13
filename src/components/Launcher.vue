<script setup lang="ts">
import { Ref, ref } from "vue";
import { invoke } from "@tauri-apps/api/tauri";
import { open } from "@tauri-apps/api/dialog";

let game_path = ref("");
let game_is_running: Ref<boolean> = ref(false)
const err_message = ref("")

function sleep(milliseconds: number) {
  return new Promise(resolve => setTimeout(resolve, milliseconds))
}

async function select_game_path()
 {
    open({directory: true}).then((res) => res ? game_path.value = res as string : err_message.value = "No Path Set" )
}

async function check_if_game_is_running() {
    invoke("is_game_running").then((res) => game_is_running.value = res as unknown as boolean)
    while(true) {
      await sleep(2000)
      invoke("is_game_running").then((res) => game_is_running.value = res as unknown as boolean)
    }
}

check_if_game_is_running()

async function inject() {
  game_is_running ?
  open().then((res) => {
    res ? invoke('inject', {path : res}).then((res) => res ? err_message.value = "DLL Injected" : err_message.value = "Failed to inject DLL" ) : err_message.value = "DLL not selected"
  }) : err_message.value = "Game is not running, start the game before injecting"
}

async function launch_game () {
  game_path.value ?
  invoke("run_game", { path: game_path.value }).then((res) => err_message.value = res as string)
  : err_message.value = "game path not set"
}
</script>

<template>
    <button v-on:click="select_game_path()">Select Game Path</button>
    <button v-on:click="inject()">Inject DLL</button>
    <button v-if="!game_is_running" v-on:click="launch_game()">Launch Game</button>

  <p>{{ game_is_running ? "Game is running (close the game before launching)" : "Game is not running" }}</p>
  <p>{{ err_message }}</p>
</template>
