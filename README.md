# Mantle2 ECS

Author: Mason Stevenson

[https://masonstevenson.dev](https://masonstevenson.dev/)

<br>

Mantle2 is an entity system for Unreal Engine. The goal for this project is to:

1) Provide some of the organizational benefits common in ECS architecture (query-by-composition, explicit tick order, per-entity operation framework).
   
2) Pick the "low hanging fruit" for performance optimizations (aka, be cache friendly, but not like *that* cache friendly).
   
3) Have simple and maintainable internals (prioritize using standard UE framework over building bespoke solutions).
   
4) Avoid the parts of ECS design that are probably irrelevant for the majority of indie devs (looking at you, dynamic composition).
   
5) Provide standard patterns for single player indie games out of the box (effect system, perception system, item system, and more).

<br>

This project is still very much **WIP**. Use at your own risk.

<br>

## Supported Engine Versions

Mantle2 currently supports **UE 5.4.**

<br>

## Required Plugins

This plugin uses my [AnankeCore](https://github.com/masonstevenson-dev/AnankeCore) plugin. This plugin is required and must be installed in order for Mantle2 to compile.

<br>

## Installation

Install this plugin in your project's plugins folder. The folder structure should be: `YourProject/Plugins/Mantle2/Mantle2/Source`.

<br>

The plugin is enabled by default.

<br>

## Usage

> [!WARNING]
>
> UNDER CONSTRUCTION