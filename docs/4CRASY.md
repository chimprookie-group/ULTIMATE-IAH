# 4Crasy

Routeur natif de modèles gratuits pour Ultimate IAH.

## API

- `GET /health` — état public minimal ;
- `GET /v1/models` — catalogue, jeton requis ;
- `POST /v1/chat/completions` — routage et fallback, jeton requis.

Écoute uniquement sur `127.0.0.1`. Le jeton est créé dans
`~/.config/Crasy/deepcrasy-h/4crasy/token` avec les permissions propriétaire.

## Canaux

- OpenRouter Free (`OPENROUTER_API_KEY`) ;
- Groq (`GROQ_API_KEY`) ;
- Gemini (`GEMINI_API_KEY`) ;
- DeepSeek (`DEEPSEEK_API_KEY`) ;
- Ollama local (`http://127.0.0.1:11434/v1`).

Les modèles sont découverts via `/models`. Les prix à zéro sont marqués
`free`. Le routage privilégie les modèles gratuits, le contexte, le type de
tâche et la priorité du canal.

## Intégration

Depuis l'interface : **Utiliser dans le harness** configure automatiquement un
provider `4crasy` avec le modèle `auto` et le jeton courant.

En headless :

```sh
ultimate-iah --router-service
ultimate-iah --router-configure
```

`--router-configure` démarre le routeur, écrit le provider dans le harness et
le rend par défaut.

## Limites

Le streaming SSE n'est pas encore exposé ; le harness utilise des réponses
complètes. Le fallback est borné à trois tentatives.
